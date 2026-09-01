#include "TextRasterizerWin.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")

namespace SwarmFormation {
namespace detail {
namespace {

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int length = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0) {
        return {};
    }
    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    if (MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
            &wide[0], length) != length) {
        return {};
    }
    return wide;
}

class ScopedGdiObject {
public:
    explicit ScopedGdiObject(HGDIOBJ object = nullptr) : object_(object) {}
    ~ScopedGdiObject() {
        if (object_ != nullptr) {
            DeleteObject(object_);
        }
    }
    HGDIOBJ get() const { return object_; }
    ScopedGdiObject(const ScopedGdiObject&) = delete;
    ScopedGdiObject& operator=(const ScopedGdiObject&) = delete;

private:
    HGDIOBJ object_;
};

class ScopedDc {
public:
    explicit ScopedDc(HDC dc = nullptr) : dc_(dc) {}
    ~ScopedDc() {
        if (dc_ != nullptr) {
            DeleteDC(dc_);
        }
    }
    HDC get() const { return dc_; }
    ScopedDc(const ScopedDc&) = delete;
    ScopedDc& operator=(const ScopedDc&) = delete;

private:
    HDC dc_;
};

} // namespace

TextRasterError RasterizeUtf8Text(
    const std::string& utf8_text,
    unsigned int requested_points,
    std::vector<RasterPoint>& output) {
    output.clear();
    const std::wstring wide = Utf8ToWide(utf8_text);
    if (wide.empty()) {
        return TextRasterError::InvalidUtf8;
    }

    // Keep the glyph deliberately low-resolution before skeletonization. A
    // very large raster preserves tiny font curves and makes a drone formation
    // look wavy. The controlled range follows the proven 60-cell text-map idea
    // while still growing enough to provide unique slots for larger fleets.
    const int font_height = std::min(
        270,
        std::max(180, static_cast<int>(std::ceil(std::sqrt(
            static_cast<double>(std::max(1u, requested_points))))) * 15));
    ScopedDc dc(CreateCompatibleDC(nullptr));
    if (dc.get() == nullptr) {
        return TextRasterError::GdiFailure;
    }

    ScopedGdiObject font(CreateFontW(
        -font_height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI"));
    if (font.get() == nullptr) {
        return TextRasterError::GdiFailure;
    }

    const HGDIOBJ previous_font = SelectObject(dc.get(), font.get());
    SIZE extent{};
    if (!GetTextExtentPoint32W(dc.get(), wide.c_str(), static_cast<int>(wide.size()), &extent)) {
        SelectObject(dc.get(), previous_font);
        return TextRasterError::GdiFailure;
    }

    const int padding = std::max(16, font_height / 8);
    const int width = std::max(64, static_cast<int>(extent.cx) + padding * 2);
    const int height = std::max(64, font_height + padding * 2);

    BITMAPINFO bitmap_info{};
    bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmap_info.bmiHeader.biWidth = width;
    bitmap_info.bmiHeader.biHeight = -height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;

    void* raw_pixels = nullptr;
    ScopedGdiObject bitmap(CreateDIBSection(
        dc.get(), &bitmap_info, DIB_RGB_COLORS, &raw_pixels, nullptr, 0));
    if (bitmap.get() == nullptr || raw_pixels == nullptr) {
        SelectObject(dc.get(), previous_font);
        return TextRasterError::GdiFailure;
    }

    const HGDIOBJ previous_bitmap = SelectObject(dc.get(), bitmap.get());
    RECT rect{0, 0, width, height};
    FillRect(dc.get(), &rect, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
    SetBkMode(dc.get(), TRANSPARENT);
    SetTextColor(dc.get(), RGB(0, 0, 0));
    DrawTextW(dc.get(), wide.c_str(), static_cast<int>(wide.size()), &rect,
              DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
    GdiFlush();

    const auto* pixels = static_cast<const std::uint32_t*>(raw_pixels);
    std::vector<RasterPoint> all_pixels;
    all_pixels.reserve(static_cast<std::size_t>(width * height / 5));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::uint32_t value = pixels[static_cast<std::size_t>(y) * width + x];
            const unsigned int blue = value & 0xFFu;
            const unsigned int green = (value >> 8u) & 0xFFu;
            const unsigned int red = (value >> 16u) & 0xFFu;
            const unsigned int luminance = (red * 299u + green * 587u + blue * 114u) / 1000u;
            if (luminance < 180u) {
                all_pixels.push_back({x, y});
            }
        }
    }

    SelectObject(dc.get(), previous_bitmap);
    SelectObject(dc.get(), previous_font);

    if (all_pixels.empty()) {
        return TextRasterError::NoVisiblePixels;
    }

    output = std::move(all_pixels);
    return TextRasterError::None;
}

} // namespace detail
} // namespace SwarmFormation
