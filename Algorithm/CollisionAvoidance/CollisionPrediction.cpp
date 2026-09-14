#include "CollisionPrediction.h"

#include <algorithm>
#include <cmath>

namespace SwarmAvoidance {
namespace detail {
namespace {

double Dot(const Vector2D& lhs, const Vector2D& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

double Length(const Vector2D& value) {
    return std::sqrt(Dot(value, value));
}

} // namespace

ClosestApproach PredictClosestApproach(
    const Vector2D& self_position,
    const Vector2D& self_velocity,
    const Vector2D& neighbor_position,
    const Vector2D& neighbor_velocity,
    double horizon) {
    const Vector2D relative_position{
        neighbor_position.x - self_position.x,
        neighbor_position.y - self_position.y
    };
    const Vector2D relative_velocity{
        neighbor_velocity.x - self_velocity.x,
        neighbor_velocity.y - self_velocity.y
    };

    const double relative_speed_squared = Dot(relative_velocity, relative_velocity);
    double closest_time = 0.0;
    if (relative_speed_squared > 1e-12) {
        closest_time = -Dot(relative_position, relative_velocity) /
            relative_speed_squared;
        closest_time = std::clamp(closest_time, 0.0, std::max(0.0, horizon));
    }

    const Vector2D closest_relative{
        relative_position.x + relative_velocity.x * closest_time,
        relative_position.y + relative_velocity.y * closest_time
    };
    return {closest_time, Length(closest_relative), closest_relative};
}

} // namespace detail
} // namespace SwarmAvoidance
