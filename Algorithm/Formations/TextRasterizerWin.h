#pragma once

#include <string>
#include <vector>

namespace SwarmFormation {
namespace detail {

struct RasterPoint {
    int x = 0;
    int y = 0;
};

enum class TextRasterError {
    None = 0,
    InvalidUtf8,
    GdiFailure,
    NoVisiblePixels
};

TextRasterError RasterizeUtf8Text(
    const std::string& utf8_text,
    unsigned int requested_points,
    std::vector<RasterPoint>& output);

} // namespace detail
} // namespace SwarmFormation
