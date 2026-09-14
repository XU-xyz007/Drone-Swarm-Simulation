#pragma once

#include "../include/CollisionAvoidanceTypes.h"

namespace SwarmAvoidance {
namespace detail {

struct ClosestApproach {
    double time = 0.0;
    double distance = 0.0;
    Vector2D relative_position;
};

ClosestApproach PredictClosestApproach(
    const Vector2D& self_position,
    const Vector2D& self_velocity,
    const Vector2D& neighbor_position,
    const Vector2D& neighbor_velocity,
    double horizon);

} // namespace detail
} // namespace SwarmAvoidance
