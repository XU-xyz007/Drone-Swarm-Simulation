#pragma once

#include "CollisionAvoidanceTypes.h"

namespace SwarmAvoidance {

// Filters C's desired velocity. It never updates position, reads DDS, or performs dynamics.
CollisionAvoidanceResult ComputeSafeVelocity(
    const CollisionAvoidanceRequest& request);

const char* AvoidanceStatusMessage(AvoidanceStatus status);

} // namespace SwarmAvoidance
