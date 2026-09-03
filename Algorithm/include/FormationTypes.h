#pragma once

#include <string>
#include <vector>

namespace SwarmFormation {

enum class FormationType : unsigned int {
    Square = 0,
    Line = 1,
    Triangle = 2,
    Circle = 3
};

enum class FormationError {
    None = 0,
    InvalidCount,
    CountTooLarge,
    InvalidSpacing,
    InvalidIndex,
    UnsupportedFormation,
    InvalidUtf8Text,
    TextTooLong,
    TextRasterizationFailed,
    InsufficientTextPoints,
    InvalidGeneratedSlot
};

struct Point2D {
    double x = 0.0;
    double y = 0.0;
};

struct FormationRequest {
    unsigned int formation_id = 0;
    std::string formation_text;
    unsigned int count = 0;
    double center_x = 0.0;
    double center_y = 0.0;
    double spacing = 0.0;
};

struct FormationResult {
    bool success = false;
    std::vector<Point2D> slots;
    FormationError error = FormationError::None;
};

} // namespace SwarmFormation
