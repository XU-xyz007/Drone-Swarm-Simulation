#pragma once

#include <vector>

#include "FormationTypes.h"

namespace SwarmFormation {
namespace detail {

std::vector<Point2D> GenerateSquareSlots(const FormationRequest& request);
std::vector<Point2D> GenerateLineSlots(const FormationRequest& request);
std::vector<Point2D> GenerateTriangleSlots(const FormationRequest& request);
std::vector<Point2D> GenerateCircleSlots(const FormationRequest& request);

} // namespace detail
} // namespace SwarmFormation
