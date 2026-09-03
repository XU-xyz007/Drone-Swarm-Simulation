#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

#define NOMINMAX
#include <windows.h>
#include <shellapi.h>

#include "../include/FormationAlgorithm.h"

namespace {

using SwarmFormation::FormationRequest;
using SwarmFormation::FormationResult;
using SwarmFormation::Point2D;

bool WriteSvg(const std::filesystem::path& path,
              const std::string& title,
              const FormationResult& result,
              bool fixed_scale = false) {
    if (!result.success || result.slots.empty()) {
        return false;
    }

    double min_x = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double min_y = std::numeric_limits<double>::max();
    double max_y = std::numeric_limits<double>::lowest();
    for (const Point2D& point : result.slots) {
        min_x = std::min(min_x, point.x);
        max_x = std::max(max_x, point.x);
        min_y = std::min(min_y, point.y);
        max_y = std::max(max_y, point.y);
    }

    constexpr double width = 720.0;
    constexpr double height = 480.0;
    constexpr double margin = 55.0;
    const double raw_span_x = max_x - min_x;
    const double raw_span_y = max_y - min_y;
    const double span_x = std::max(1.0, raw_span_x);
    const double span_y = std::max(1.0, raw_span_y);
    const double scale = fixed_scale
        ? 8.0
        : std::min((width - margin * 2.0) / span_x,
                   (height - margin * 2.0) / span_y);

    std::vector<double> nearest(result.slots.size(),
                                std::numeric_limits<double>::max());
    for (std::size_t i = 0; i < result.slots.size(); ++i) {
        for (std::size_t j = i + 1; j < result.slots.size(); ++j) {
            const double dx = result.slots[i].x - result.slots[j].x;
            const double dy = result.slots[i].y - result.slots[j].y;
            const double distance = std::hypot(dx, dy);
            nearest[i] = std::min(nearest[i], distance);
            nearest[j] = std::min(nearest[j], distance);
        }
    }
    const double nearest_mean = result.slots.size() > 1
        ? std::accumulate(nearest.begin(), nearest.end(), 0.0) / nearest.size()
        : 0.0;
    double variance = 0.0;
    if (result.slots.size() > 1) {
        for (const double value : nearest) {
            variance += (value - nearest_mean) * (value - nearest_mean);
        }
        variance /= nearest.size();
    }
    const double nearest_min = result.slots.size() > 1
        ? *std::min_element(nearest.begin(), nearest.end()) : 0.0;
    const double nearest_max = result.slots.size() > 1
        ? *std::max_element(nearest.begin(), nearest.end()) : 0.0;

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    out << std::fixed << std::setprecision(2);
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"720\" height=\"480\" viewBox=\"0 0 720 480\">\n";
    out << "<rect width=\"720\" height=\"480\" fill=\"#f7f9fc\"/>\n";
    out << "<text x=\"28\" y=\"34\" font-family=\"Microsoft YaHei, sans-serif\" font-size=\"20\" font-weight=\"700\" fill=\"#1f4d78\">"
        << title << "</text>\n";
    out << "<text x=\"692\" y=\"34\" text-anchor=\"end\" font-family=\"Arial\" font-size=\"13\" fill=\"#667085\">N="
        << result.slots.size() << "</text>\n";
    out << "<text x=\"28\" y=\"54\" font-family=\"Arial\" font-size=\"11\" fill=\"#667085\">"
        << (fixed_scale ? "Fixed scale 8 px/m" : "Auto fit")
        << " | size=" << raw_span_x << "m x " << raw_span_y
        << "m | NN mean=" << nearest_mean << "m, sd=" << std::sqrt(variance)
        << "m, max/min=" << (nearest_min > 0.0 ? nearest_max / nearest_min : 0.0)
        << "</text>\n";
    const double point_radius = result.slots.size() >= 150 ? 4.0
        : (result.slots.size() >= 80 ? 4.7 : 5.5);
    for (std::size_t i = 0; i < result.slots.size(); ++i) {
        const Point2D& point = result.slots[i];
        const double x = fixed_scale
            ? width / 2.0 + (point.x - (min_x + max_x) * 0.5) * scale
            : (raw_span_x <= 1e-12 ? width / 2.0
                : margin + (point.x - min_x) * scale);
        const double y = fixed_scale
            ? height / 2.0 - (point.y - (min_y + max_y) * 0.5) * scale
            : (raw_span_y <= 1e-12 ? height / 2.0
                : height - margin - (point.y - min_y) * scale);
        out << "<circle cx=\"" << x << "\" cy=\"" << y
            << "\" r=\"" << point_radius
            << "\" fill=\"#2e74b5\" stroke=\"#ffffff\" stroke-width=\"1.2\"/>\n";
    }
    out << "</svg>\n";
    return true;
}

bool Generate(const std::filesystem::path& output_dir,
              const std::string& filename,
              const std::string& title,
              unsigned int formation_id,
              const std::string& text,
              unsigned int count) {
    FormationRequest request;
    request.formation_id = formation_id;
    request.formation_text = text;
    request.count = count;
    request.center_x = 0.0;
    request.center_y = 0.0;
    request.spacing = 5.0;
    const FormationResult result = SwarmFormation::GenerateSlots(request);
    if (!result.success) {
        std::cerr << filename << ": " << SwarmFormation::FormationErrorMessage(result.error) << '\n';
        return false;
    }
    return WriteSvg(output_dir / filename, title, result);
}

bool ReadUnsigned(const char* prompt, unsigned int& value, bool allow_zero = false) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        return false;
    }
    try {
        std::size_t used = 0;
        const unsigned long parsed = std::stoul(line, &used);
        if (used != line.size() || (!allow_zero && parsed == 0) ||
            parsed > std::numeric_limits<unsigned int>::max()) {
            return false;
        }
        value = static_cast<unsigned int>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool ReadDouble(const char* prompt, double& value) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        return false;
    }
    try {
        std::size_t used = 0;
        const double parsed = std::stod(line, &used);
        if (used != line.size() || parsed <= 0.0) {
            return false;
        }
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

int RunInteractivePreview() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::cout << u8"\n========== B模块编队算法手动测试台 ==========\n"
              << u8"0 - 方阵\n"
              << u8"1 - 一字队形\n"
              << u8"2 - 三角形\n"
              << u8"3 - 圆形\n"
              << u8"4 - 文字队形\n\n";

    unsigned int choice = 0;
    unsigned int count = 0;
    double spacing = 0.0;
    if (!ReadUnsigned(u8"请输入队形编号（0～4）：", choice, true) || choice > 4) {
        std::cerr << u8"输入错误：队形编号只能是0～4。\n";
        return 1;
    }
    if (!ReadUnsigned(u8"请输入无人机数量（大于0）：", count)) {
        std::cerr << u8"输入错误：无人机数量必须是正整数。\n";
        return 1;
    }
    if (!ReadDouble(u8"请输入相邻槽位间距（例如5）：", spacing)) {
        std::cerr << u8"输入错误：间距必须是大于0的数字。\n";
        return 1;
    }

    std::string text;
    if (choice == 4) {
        std::cout << u8"请输入要显示的文字（例如：中、AI、无人机）：";
        if (!std::getline(std::cin, text) || text.empty()) {
            std::cerr << u8"输入错误：文字不能为空。\n";
            return 1;
        }
        const std::size_t character_count = static_cast<std::size_t>(std::count_if(
            text.begin(), text.end(), [](char byte) {
                return (static_cast<unsigned char>(byte) & 0xC0u) != 0x80u;
            }));
        const unsigned int recommended_count = static_cast<unsigned int>(
            std::max<std::size_t>(1, character_count) * 40u);
        if (count < recommended_count) {
            std::cout << u8"提示：当前无人机数量较少，只适合验证接口和数量。"
                      << u8"若要清晰展示这段文字，建议至少使用 "
                      << recommended_count << u8" 架无人机。\n";
        }
    }

    FormationRequest request;
    request.formation_id = choice == 4 ? 0 : choice;
    request.formation_text = text;
    request.count = count;
    request.center_x = 0.0;
    request.center_y = 0.0;
    request.spacing = spacing;
    const FormationResult result = SwarmFormation::GenerateSlots(request);
    if (!result.success) {
        std::cerr << u8"生成失败：" << SwarmFormation::FormationErrorMessage(result.error) << '\n';
        return 1;
    }

    const std::filesystem::path output_dir =
        std::filesystem::current_path() / "formation-preview";
    std::filesystem::create_directories(output_dir);
    const std::filesystem::path output_path = output_dir / "manual-preview.svg";
    const std::filesystem::path fixed_output_path =
        output_dir / "manual-preview-fixed-scale.svg";
    const std::string title = choice == 4 ? "Text / " + text : "Manual formation preview";
    if (!WriteSvg(output_path, title, result)) {
        std::cerr << u8"预览文件写入失败。\n";
        return 1;
    }
    if (!WriteSvg(fixed_output_path, title, result, true)) {
        std::cerr << u8"固定比例尺预览文件写入失败。\n";
        return 1;
    }

    std::cout << u8"\n生成成功，共 " << result.slots.size() << u8" 个槽位。\n"
              << u8"预览文件：" << output_path.u8string() << u8"\n"
              << u8"固定比例尺：" << fixed_output_path.u8string() << u8"\n"
              << u8"浏览器即将自动打开预览。关闭本窗口即可结束测试。\n";
    ShellExecuteW(nullptr, L"open", output_path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 1) {
        return RunInteractivePreview();
    }

    const std::filesystem::path output_dir = argc > 1
        ? std::filesystem::u8path(argv[1])
        : std::filesystem::current_path() / "formation-preview";
    std::filesystem::create_directories(output_dir);

    bool success = true;
    success = Generate(output_dir, "square.svg", "Square / 方阵", 0, "", 20) && success;
    success = Generate(output_dir, "line.svg", "Line / 一字", 1, "", 20) && success;
    success = Generate(output_dir, "triangle.svg", "Triangle / 三角", 2, "", 20) && success;
    success = Generate(output_dir, "circle.svg", "Circle / 圆形", 3, "", 20) && success;
    success = Generate(output_dir, "text-ai.svg", "Text / AI", 0, u8"AI", 50) && success;
    success = Generate(output_dir, "text-zhong.svg", "Text / 中", 0, u8"中", 50) && success;
    success = Generate(output_dir, "text-uav.svg", "Text / 无人机", 0, u8"无人机", 120) && success;

    if (!success) {
        return 1;
    }
    std::cout << "Formation previews written to: " << output_dir.u8string() << '\n';
    return 0;
}
