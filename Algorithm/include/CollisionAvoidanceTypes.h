#pragma once

#include <vector>

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

struct CollisionAvoidanceConfig {
    double safe_distance = 1.0;
    double warning_distance = 1.5;
    double prediction_horizon = 1.5;
    double max_speed = 5.0;
    double max_acceleration = 2.0;
    double max_deceleration = 3.0;
    double max_lateral_acceleration = 2.5;
    double arrival_tolerance = 0.05;
    double stopped_speed_tolerance = 0.05;
    double avoidance_gain = 1.0;
};

struct CollisionAvoidanceRequest {
    unsigned long drone_id = 0;
    Vector2D current_position;
    Vector2D current_velocity;
    Vector2D desired_velocity;
    Vector2D target_position;
    std::vector<NeighborState> neighbors;
    double dt = 0.1;
    CollisionAvoidanceConfig config;
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
