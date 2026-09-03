#pragma once

#include <string>
#include <vector>

#include "TextRasterizerWin.h"

namespace SwarmFormation {
namespace detail {

// Loads the MIT-licensed Chinese Hershey single-line font and converts UTF-8
// text into dense points along its vector strokes. Returns false when the font
// asset or any requested glyph is unavailable.
bool BuildHersheyTextCandidates(
    const std::string& utf8_text,
    std::vector<RasterPoint>& output,
    std::vector<RasterPoint>* anchors = nullptr);

// Produces at most requested_count stroke-aware seed points. Every stroke gets
// a representative whenever the fleet is large enough, then remaining drones
// are allocated by stroke length and sampled by arc length.
bool BuildHersheyTextUniformSeeds(
    const std::string& utf8_text,
    unsigned int requested_count,
    std::vector<RasterPoint>& output);

} // namespace detail
} // namespace SwarmFormation
