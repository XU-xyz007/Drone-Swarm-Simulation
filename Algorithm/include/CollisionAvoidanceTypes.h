#pragma once

#include <vector>

#include "SwarmConfig.h"

namespace SwarmAvoidance {

struct Vector2D {
    double x = 0.0;
    double y = 0.0;
};

enum class NeighborMotionState {
    Unknown = 0,
    Moving,
    Arrived
};

struct NeighborState {
    unsigned long drone_id = 0;
    Vector2D position;
    Vector2D velocity;
    NeighborMotionState motion_state = NeighborMotionState::Unknown;
};

struct CollisionAvoidanceRequest {
    unsigned long drone_id = 0;
    Vector2D current_position;
    Vector2D current_velocity;
    Vector2D desired_velocity;
    Vector2D target_position;
    std::vector<NeighborState> neighbors;
    // Snapshot of the single shared C/B physical configuration.
    V2Config config;

    // The caller must pass the measured control-loop period every cycle.
    double dt = 0.0;
    double communication_delay = 0.0;
    double arrival_tolerance = 0.05;
    double stopped_speed_tolerance = 0.05;
};

enum class AvoidanceStatus {
    Safe = 0,
    Avoiding,
    Arrived,
    InvalidInput
};

struct CollisionAvoidanceResult {
    Vector2D safe_velocity;
    bool modified = false;
    AvoidanceStatus status = AvoidanceStatus::Safe;
    double predicted_min_distance = 0.0;
};

} // namespace SwarmAvoidance
