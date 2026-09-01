#include "HersheyFont.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace SwarmFormation {
namespace detail {
namespace {

struct HersheyPoint {
    int x = 0;
    int y = 0;
};

struct HersheyGlyph {
    int minimum_x = 0;
    int maximum_x = 0;
    std::vector<std::vector<HersheyPoint>> strokes;
};

std::vector<std::uint32_t> DecodeUtf8(const std::string& text) {
    std::vector<std::uint32_t> codepoints;
    for (std::size_t i = 0; i < text.size();) {
        const unsigned char first = static_cast<unsigned char>(text[i]);
        std::uint32_t value = 0;
        std::size_t length = 0;
        if (first < 0x80u) {
            value = first;
            length = 1;
        } else if ((first & 0xE0u) == 0xC0u) {
            value = first & 0x1Fu;
            length = 2;
        } else if ((first & 0xF0u) == 0xE0u) {
            value = first & 0x0Fu;
            length = 3;
        } else if ((first & 0xF8u) == 0xF0u) {
            value = first & 0x07u;
            length = 4;
        } else {
            return {};
        }
        if (i + length > text.size()) {
            return {};
        }
        for (std::size_t j = 1; j < length; ++j) {
            const unsigned char continuation =
                static_cast<unsigned char>(text[i + j]);
            if ((continuation & 0xC0u) != 0x80u) {
                return {};
            }
            value = (value << 6u) | (continuation & 0x3Fu);
        }
        codepoints.push_back(value);
        i += length;
    }
    return codepoints;
}

std::filesystem::path ExecutableDirectory() {
    std::wstring path(32768, L'\0');
    const DWORD length = GetModuleFileNameW(
        nullptr, &path[0], static_cast<DWORD>(path.size()));
    if (length == 0 || length >= path.size()) {
        return {};
    }
    path.resize(length);
    return std::filesystem::path(path).parent_path();
}

std::filesystem::path FindFontAsset() {
    const std::filesystem::path relative =
        std::filesystem::path("assets") / "chinese-hershey-font" / "Heiti.hf.txt";
    const std::filesystem::path executable_candidate = ExecutableDirectory() / relative;
    if (std::filesystem::exists(executable_candidate)) {
        return executable_candidate;
    }
    const std::filesystem::path source_candidate =
        std::filesystem::path(__FILE__).parent_path() / relative;
    if (std::filesystem::exists(source_candidate)) {
        return source_candidate;
    }
    const std::filesystem::path working_candidate =
        std::filesystem::current_path() / "SharedMessage" / "Algorithm" / relative;
    return std::filesystem::exists(working_candidate)
        ? working_candidate
        : std::filesystem::path{};
}

std::unordered_map<std::uint32_t, HersheyGlyph> LoadFont() {
    std::unordered_map<std::uint32_t, HersheyGlyph> glyphs;
    const std::filesystem::path path = FindFontAsset();
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return glyphs;
    }

    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.size() < 10) {
            continue;
        }
        std::uint32_t codepoint = 0;
        try {
            codepoint = static_cast<std::uint32_t>(std::stoul(line.substr(0, 5)));
        } catch (...) {
            continue;
        }
        const std::string entry = line.substr(5);
        if (entry.size() < 5) {
            continue;
        }

        HersheyGlyph glyph;
        glyph.minimum_x = static_cast<unsigned char>(entry[3]) - 'R';
        glyph.maximum_x = static_cast<unsigned char>(entry[4]) - 'R';
        glyph.strokes.emplace_back();
        for (std::size_t i = 5; i + 1 < entry.size(); i += 2) {
            if (entry[i] == ' ' && entry[i + 1] == 'R') {
                if (!glyph.strokes.back().empty()) {
                    glyph.strokes.emplace_back();
                }
                continue;
            }
            glyph.strokes.back().push_back({
                static_cast<unsigned char>(entry[i]) - 'R',
                static_cast<unsigned char>(entry[i + 1]) - 'R'
            });
        }
        if (glyph.strokes.back().empty()) {
            glyph.strokes.pop_back();
        }
        if (!glyph.strokes.empty()) {
            glyphs.emplace(codepoint, std::move(glyph));
        }
    }
    return glyphs;
}

const std::unordered_map<std::uint32_t, HersheyGlyph>& Font() {
    static std::unordered_map<std::uint32_t, HersheyGlyph> glyphs;
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    // Do not permanently cache a failed lookup. This allows a process to
    // recover after the font asset is copied or its working directory changes.
    if (glyphs.empty()) {
        glyphs = LoadFont();
    }
    return glyphs;
}

} // namespace

bool BuildHersheyTextUniformSeeds(
    const std::string& utf8_text,
    unsigned int requested_count,
    std::vector<RasterPoint>& output) {
    output.clear();
    if (requested_count == 0) {
        return false;
    }

    struct Path {
        std::vector<std::pair<double, double>> points;
        double length = 0.0;
        unsigned int samples = 0;
    };
    const std::vector<std::uint32_t> codepoints = DecodeUtf8(utf8_text);
    if (codepoints.empty() || Font().empty()) {
        return false;
    }

    std::vector<Path> paths;
    double cursor_x = 0.0;
    for (const std::uint32_t codepoint : codepoints) {
        const auto found = Font().find(codepoint);
        if (found == Font().end()) {
            output.clear();
            return false;
        }
        const HersheyGlyph& glyph = found->second;
        for (const auto& stroke : glyph.strokes) {
            if (stroke.size() < 2) {
                continue;
            }
            Path path;
            for (const HersheyPoint& point : stroke) {
                path.points.emplace_back(
                    cursor_x + point.x - glyph.minimum_x,
                    static_cast<double>(point.y));
            }
            for (std::size_t i = 1; i < path.points.size(); ++i) {
                const double dx = path.points[i].first - path.points[i - 1].first;
                const double dy = path.points[i].second - path.points[i - 1].second;
                path.length += std::hypot(dx, dy);
            }
            if (path.length > 1e-9) {
                paths.push_back(std::move(path));
            }
        }
        cursor_x += glyph.maximum_x - glyph.minimum_x;
    }
    if (paths.empty()) {
        return false;
    }

    // Too few samples per stroke destroy recognisability for text with many
    // strokes at a modest drone count. Uniform per-stroke sampling is only
    // appropriate once every stroke can receive roughly four representatives.
    // Returning false selects the corner/outline
    // preserving low-density strategy in TextFormation.cpp.
    if (requested_count < paths.size() * 4u) {
        return false;
    }

    // Give every stroke one representative before distributing the remaining
    // drones.  Each additional drone goes where it most reduces the nominal
    // arc-length interval (length / sample count).
    unsigned int assigned = 0;
    if (requested_count >= paths.size()) {
        for (Path& path : paths) {
            path.samples = 1;
            ++assigned;
        }
    }
    while (assigned < requested_count) {
        std::size_t best = 0;
        double best_interval = -1.0;
        for (std::size_t i = 0; i < paths.size(); ++i) {
            const double interval = paths[i].length /
                static_cast<double>(paths[i].samples + 1u);
            if (interval > best_interval) {
                best_interval = interval;
                best = i;
            }
        }
        ++paths[best].samples;
        ++assigned;
    }

    double total_length = 0.0;
    for (const Path& path : paths) {
        total_length += path.length;
    }
    constexpr double kCoordinateScale = 8.0;
    for (const Path& path : paths) {
        for (unsigned int sample = 0; sample < path.samples; ++sample) {
            // Mid-cell positions make every sample own the same amount of
            // stroke and avoid crowding shared endpoints/intersections.
            const double target = path.length *
                (static_cast<double>(sample) + 0.5) / path.samples;
            double walked = 0.0;
            for (std::size_t i = 1; i < path.points.size(); ++i) {
                const double dx = path.points[i].first - path.points[i - 1].first;
                const double dy = path.points[i].second - path.points[i - 1].second;
                const double segment = std::hypot(dx, dy);
                if (target <= walked + segment || i + 1 == path.points.size()) {
                    const double t = segment > 1e-12
                        ? std::clamp((target - walked) / segment, 0.0, 1.0)
                        : 0.0;
                    output.push_back({
                        static_cast<int>(std::lround(
                            (path.points[i - 1].first + dx * t) * kCoordinateScale)),
                        static_cast<int>(std::lround(
                            (path.points[i - 1].second + dy * t) * kCoordinateScale))
                    });
                    break;
                }
                walked += segment;
            }
        }
    }

    // Merge samples that are close only because separate strokes cross or
    // touch. Their released slots are later placed into the largest remaining
    // gaps by SelectFarthestPointsWithAnchors.
    const double nominal_interval = total_length /
        static_cast<double>(requested_count) * kCoordinateScale;
    const double merge_distance_squared =
        nominal_interval * nominal_interval * 0.30;
    std::vector<RasterPoint> separated;
    separated.reserve(output.size());
    for (const RasterPoint& candidate : output) {
        bool conflicts = false;
        for (const RasterPoint& kept : separated) {
            const double dx = static_cast<double>(candidate.x - kept.x);
            const double dy = static_cast<double>(candidate.y - kept.y);
            if (dx * dx + dy * dy < merge_distance_squared) {
                conflicts = true;
                break;
            }
        }
        if (!conflicts) {
            separated.push_back(candidate);
        }
    }
    output = std::move(separated);
    std::sort(output.begin(), output.end(),
              [](const RasterPoint& lhs, const RasterPoint& rhs) {
                  return lhs.y < rhs.y || (lhs.y == rhs.y && lhs.x < rhs.x);
              });
    output.erase(std::unique(output.begin(), output.end(),
        [](const RasterPoint& lhs, const RasterPoint& rhs) {
            return lhs.x == rhs.x && lhs.y == rhs.y;
        }), output.end());
    return !output.empty();
}

bool BuildHersheyTextCandidates(
    const std::string& utf8_text,
    std::vector<RasterPoint>& output,
    std::vector<RasterPoint>* anchors) {
    output.clear();
    if (anchors != nullptr) {
        anchors->clear();
    }
    const std::vector<std::uint32_t> codepoints = DecodeUtf8(utf8_text);
    if (codepoints.empty() || Font().empty()) {
        return false;
    }

    constexpr double kCoordinateScale = 8.0;
    constexpr double kSamplesPerFontUnit = 4.0;
    double cursor_x = 0.0;
    for (std::uint32_t codepoint : codepoints) {
        const auto found = Font().find(codepoint);
        if (found == Font().end()) {
            output.clear();
            return false;
        }
        const HersheyGlyph& glyph = found->second;
        for (const std::vector<HersheyPoint>& stroke : glyph.strokes) {
            if (anchors != nullptr && !stroke.empty()) {
                auto append_anchor = [&](const HersheyPoint& point) {
                    anchors->push_back({
                        static_cast<int>(std::lround(
                            (cursor_x + point.x - glyph.minimum_x) * kCoordinateScale)),
                        static_cast<int>(std::lround(point.y * kCoordinateScale))
                    });
                };
                append_anchor(stroke.front());
                for (std::size_t i = 1; i + 1 < stroke.size(); ++i) {
                    const double ax = stroke[i].x - stroke[i - 1].x;
                    const double ay = stroke[i].y - stroke[i - 1].y;
                    const double bx = stroke[i + 1].x - stroke[i].x;
                    const double by = stroke[i + 1].y - stroke[i].y;
                    const double a_length = std::sqrt(ax * ax + ay * ay);
                    const double b_length = std::sqrt(bx * bx + by * by);
                    if (a_length <= 1e-12 || b_length <= 1e-12) {
                        continue;
                    }
                    const double cosine = (ax * bx + ay * by) / (a_length * b_length);
                    // Preserve only a visually meaningful corner (roughly 25
                    // degrees or sharper), not every control point on a curve.
                    if (cosine < 0.906307787) {
                        append_anchor(stroke[i]);
                    }
                }
                if (stroke.size() > 1) {
                    append_anchor(stroke.back());
                }
            }
            for (std::size_t i = 1; i < stroke.size(); ++i) {
                const double start_x = cursor_x + stroke[i - 1].x - glyph.minimum_x;
                const double start_y = stroke[i - 1].y;
                const double end_x = cursor_x + stroke[i].x - glyph.minimum_x;
                const double end_y = stroke[i].y;
                const double dx = end_x - start_x;
                const double dy = end_y - start_y;
                const int steps = std::max(
                    1, static_cast<int>(std::ceil(
                        std::sqrt(dx * dx + dy * dy) * kSamplesPerFontUnit)));
                for (int step = 0; step <= steps; ++step) {
                    const double t = static_cast<double>(step) / steps;
                    output.push_back({
                        static_cast<int>(std::lround((start_x + dx * t) * kCoordinateScale)),
                        static_cast<int>(std::lround((start_y + dy * t) * kCoordinateScale))
                    });
                }
            }
        }
        cursor_x += glyph.maximum_x - glyph.minimum_x;
    }

    std::sort(output.begin(), output.end(),
              [](const RasterPoint& lhs, const RasterPoint& rhs) {
                  return lhs.y < rhs.y || (lhs.y == rhs.y && lhs.x < rhs.x);
              });
    output.erase(
        std::unique(output.begin(), output.end(),
                    [](const RasterPoint& lhs, const RasterPoint& rhs) {
                        return lhs.x == rhs.x && lhs.y == rhs.y;
                    }),
        output.end());
    if (anchors != nullptr) {
        constexpr double kAnchorMergeDistance = 2.25 * kCoordinateScale;
        constexpr double kAnchorMergeDistanceSquared =
            kAnchorMergeDistance * kAnchorMergeDistance;
        std::vector<RasterPoint> separated;
        separated.reserve(anchors->size());
        for (const RasterPoint& candidate : *anchors) {
            bool too_close = false;
            for (const RasterPoint& kept : separated) {
                const double dx = static_cast<double>(candidate.x - kept.x);
                const double dy = static_cast<double>(candidate.y - kept.y);
                if (dx * dx + dy * dy < kAnchorMergeDistanceSquared) {
                    too_close = true;
                    break;
                }
            }
            if (!too_close) {
                separated.push_back(candidate);
            }
        }
        *anchors = std::move(separated);
        std::sort(anchors->begin(), anchors->end(),
                  [](const RasterPoint& lhs, const RasterPoint& rhs) {
                      return lhs.y < rhs.y || (lhs.y == rhs.y && lhs.x < rhs.x);
                  });
        anchors->erase(
            std::unique(anchors->begin(), anchors->end(),
                        [](const RasterPoint& lhs, const RasterPoint& rhs) {
                            return lhs.x == rhs.x && lhs.y == rhs.y;
                        }),
            anchors->end());
    }
    return !output.empty();
}

} // namespace detail
} // namespace SwarmFormation
