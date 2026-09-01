#pragma once

#include <vector>

#include "FormationTypes.h"

namespace SwarmFormation {
namespace detail {

double DistanceSquared(const Point2D& lhs, const Point2D& rhs);
double MinimumPairDistance(const std::vector<Point2D>& points);
bool IsFinitePoint(const Point2D& point);
bool HasDuplicateSlots(const std::vector<Point2D>& points, double tolerance = 1e-9);
void CenterBoundingBox(std::vector<Point2D>& points);
void ScalePoints(std::vector<Point2D>& points, double scale);
void TranslatePoints(std::vector<Point2D>& points, double dx, double dy);
void SortSlotsStable(std::vector<Point2D>& points);

} // namespace detail
} // namespace SwarmFormation
