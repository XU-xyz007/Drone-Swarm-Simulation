#include "GeometryUtils.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace SwarmFormation {
namespace detail {

double DistanceSquared(const Point2D& lhs, const Point2D& rhs) {
    const double dx = lhs.x - rhs.x;
    const double dy = lhs.y - rhs.y;
    return dx * dx + dy * dy;
}

double MinimumPairDistance(const std::vector<Point2D>& points) {
    if (points.size() < 2) {
        return 0.0;
    }

    double minimum_squared = std::numeric_limits<double>::max();
    for (std::size_t i = 0; i < points.size(); ++i) {
        for (std::size_t j = i + 1; j < points.size(); ++j) {
            minimum_squared = std::min(minimum_squared, DistanceSquared(points[i], points[j]));
        }
    }
    return std::sqrt(minimum_squared);
}

bool IsFinitePoint(const Point2D& point) {
    return std::isfinite(point.x) && std::isfinite(point.y);
}

bool HasDuplicateSlots(const std::vector<Point2D>& points, double tolerance) {
    const double tolerance_squared = tolerance * tolerance;
    for (std::size_t i = 0; i < points.size(); ++i) {
        for (std::size_t j = i + 1; j < points.size(); ++j) {
            if (DistanceSquared(points[i], points[j]) <= tolerance_squared) {
                return true;
            }
        }
    }
    return false;
}

void CenterBoundingBox(std::vector<Point2D>& points) {
    if (points.empty()) {
        return;
    }

    double min_x = points.front().x;
    double max_x = points.front().x;
    double min_y = points.front().y;
    double max_y = points.front().y;
    for (const Point2D& point : points) {
        min_x = std::min(min_x, point.x);
        max_x = std::max(max_x, point.x);
        min_y = std::min(min_y, point.y);
        max_y = std::max(max_y, point.y);
    }

    const double center_x = (min_x + max_x) * 0.5;
    const double center_y = (min_y + max_y) * 0.5;
    TranslatePoints(points, -center_x, -center_y);
}

void ScalePoints(std::vector<Point2D>& points, double scale) {
    for (Point2D& point : points) {
        point.x *= scale;
        point.y *= scale;
    }
}

void TranslatePoints(std::vector<Point2D>& points, double dx, double dy) {
    for (Point2D& point : points) {
        point.x += dx;
        point.y += dy;
    }
}

void SortSlotsStable(std::vector<Point2D>& points) {
    std::stable_sort(points.begin(), points.end(), [](const Point2D& lhs, const Point2D& rhs) {
        constexpr double epsilon = 1e-12;
        if (std::fabs(lhs.y - rhs.y) > epsilon) {
            return lhs.y > rhs.y;
        }
        return lhs.x < rhs.x;
    });
}

} // namespace detail
} // namespace SwarmFormation
