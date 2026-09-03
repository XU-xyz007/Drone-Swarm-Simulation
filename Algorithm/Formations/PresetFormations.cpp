#include "PresetFormations.h"

#include <algorithm>
#include <cmath>

#include "../Math/GeometryUtils.h"

namespace SwarmFormation {
namespace detail {
namespace {

constexpr double kPi = 3.1415926535897932384626433832795;

void FinishPreset(std::vector<Point2D>& slots, const FormationRequest& request) {
    CenterBoundingBox(slots);
    TranslatePoints(slots, request.center_x, request.center_y);
}

} // namespace

std::vector<Point2D> GenerateLineSlots(const FormationRequest& request) {
    if (request.count == 0) {
        return {};
    }
    std::vector<Point2D> slots;
    slots.reserve(request.count);
    const double center_offset = (static_cast<double>(request.count) - 1.0) * 0.5;
    for (unsigned int i = 0; i < request.count; ++i) {
        slots.push_back({
            request.center_x + (static_cast<double>(i) - center_offset) * request.spacing,
            request.center_y
        });
    }
    return slots;
}

std::vector<Point2D> GenerateCircleSlots(const FormationRequest& request) {
    if (request.count == 0) {
        return {};
    }
    if (request.count == 1) {
        return {{request.center_x, request.center_y}};
    }

    std::vector<Point2D> slots;
    slots.reserve(request.count);
    const double radius = request.spacing / (2.0 * std::sin(kPi / request.count));
    for (unsigned int i = 0; i < request.count; ++i) {
        const double theta = 2.0 * kPi * static_cast<double>(i) / request.count;
        slots.push_back({
            request.center_x + radius * std::cos(theta),
            request.center_y + radius * std::sin(theta)
        });
    }
    return slots;
}

std::vector<Point2D> GenerateSquareSlots(const FormationRequest& request) {
    if (request.count == 0) {
        return {};
    }
    const unsigned int columns = static_cast<unsigned int>(
        std::ceil(std::sqrt(static_cast<double>(request.count))));

    std::vector<Point2D> slots;
    slots.reserve(request.count);
    for (unsigned int i = 0; i < request.count; ++i) {
        const unsigned int row = i / columns;
        const unsigned int column = i % columns;
        slots.push_back({
            static_cast<double>(column) * request.spacing,
            -static_cast<double>(row) * request.spacing
        });
    }
    FinishPreset(slots, request);
    return slots;
}

std::vector<Point2D> GenerateTriangleSlots(const FormationRequest& request) {
    if (request.count == 0) {
        return {};
    }
    std::vector<Point2D> slots;
    slots.reserve(request.count);
    const double row_height = request.spacing * std::sqrt(3.0) * 0.5;

    unsigned int generated = 0;
    unsigned int row = 0;
    while (generated < request.count) {
        const unsigned int capacity = row + 1;
        const unsigned int in_row = std::min(capacity, request.count - generated);
        const double full_row_width = static_cast<double>(capacity - 1) * request.spacing;
        const double left = -full_row_width * 0.5;
        for (unsigned int column = 0; column < in_row; ++column) {
            slots.push_back({
                left + static_cast<double>(column) * request.spacing,
                -static_cast<double>(row) * row_height
            });
            ++generated;
        }
        ++row;
    }

    FinishPreset(slots, request);
    return slots;
}

} // namespace detail
} // namespace SwarmFormation
