#include "../include/CollisionAvoidance.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "CollisionPrediction.h"

namespace SwarmAvoidance {
namespace {

constexpr double kEpsilon = 1e-9;

double Dot(const Vector2D& lhs, const Vector2D& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

double Length(const Vector2D& value) {
    return std::sqrt(Dot(value, value));
}

Vector2D Add(const Vector2D& lhs, const Vector2D& rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

Vector2D Subtract(const Vector2D& lhs, const Vector2D& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

Vector2D Scale(const Vector2D& value, double factor) {
    return {value.x * factor, value.y * factor};
}

Vector2D NormalizeOr(const Vector2D& value, const Vector2D& fallback) {
    const double length = Length(value);
    if (length <= kEpsilon) {
        return fallback;
    }
    return Scale(value, 1.0 / length);
}

Vector2D ClampMagnitude(const Vector2D& value, double maximum) {
    const double length = Length(value);
    if (length <= maximum || length <= kEpsilon) {
        return value;
    }
    return Scale(value, maximum / length);
}

Vector2D EffectiveNeighborVelocity(const NeighborState& neighbor) {
    // An arrived drone owns its slot and must not be asked to yield. Treat it as
    // stationary even if its last telemetry sample still contains residual speed.
    return neighbor.motion_state == NeighborMotionState::Arrived
        ? Vector2D{0.0, 0.0}
        : neighbor.velocity;
}

bool IsFinite(const Vector2D& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

bool IsValidConfig(const V2Config& config) {
    return std::isfinite(config.avoidance_params.drone_radius) && config.avoidance_params.drone_radius > 0.0 &&
        std::isfinite(config.avoidance_params.safety_margin) && config.avoidance_params.safety_margin >= 0.0 &&
        std::isfinite(config.dynamics_limits_2d.v_max) && config.dynamics_limits_2d.v_max > 0.0 &&
        std::isfinite(config.dynamics_limits_2d.a_max_acc) && config.dynamics_limits_2d.a_max_acc > 0.0 &&
        std::isfinite(config.dynamics_limits_2d.a_max_dec) && config.dynamics_limits_2d.a_max_dec > 0.0 &&
        std::isfinite(config.dynamics_limits_2d.a_max_lat) &&
        config.dynamics_limits_2d.a_max_lat > 0.0;
}

bool IsValidRequest(const CollisionAvoidanceRequest& request) {
    if (!std::isfinite(request.dt) || request.dt <= 0.0 ||
        !IsFinite(request.current_position) ||
        !IsFinite(request.current_velocity) ||
        !IsFinite(request.desired_velocity) ||
        !IsFinite(request.target_position) ||
        !std::isfinite(request.communication_delay) ||
        request.communication_delay < 0.0 ||
        !std::isfinite(request.arrival_tolerance) ||
        request.arrival_tolerance < 0.0 ||
        !std::isfinite(request.stopped_speed_tolerance) ||
        request.stopped_speed_tolerance < 0.0 ||
        !IsValidConfig(request.config)) {
        return false;
    }
    for (const NeighborState& neighbor : request.neighbors) {
        if (neighbor.drone_id == request.drone_id ||
            !IsFinite(neighbor.position) || !IsFinite(neighbor.velocity)) {
            return false;
        }
    }
    return true;
}

double BaseSafetyDistance(const CollisionAvoidanceRequest& request) {
    return 2.0 * request.config.avoidance_params.drone_radius + request.config.avoidance_params.safety_margin;
}

double PlanningSafetyDistance(const CollisionAvoidanceRequest& request) {
    // Reserve one worst-case sample of travel so discrete integration and a
    // delayed telemetry sample cannot step across the physical boundary.
    return BaseSafetyDistance(request) + request.config.avoidance_params.drone_radius +
        2.0 * request.config.dynamics_limits_2d.v_max *
            (request.dt + request.communication_delay);
}

double ClosingSpeed(
    const CollisionAvoidanceRequest& request,
    const NeighborState& neighbor,
    const Vector2D& self_velocity) {
    const Vector2D separation = Subtract(neighbor.position, request.current_position);
    const Vector2D line_of_sight = NormalizeOr(separation, {1.0, 0.0});
    const Vector2D relative_velocity = Subtract(
        self_velocity, EffectiveNeighborVelocity(neighbor));
    return std::max(0.0, Dot(relative_velocity, line_of_sight));
}

double AvailableRelativeDeceleration(
    const CollisionAvoidanceRequest& request,
    const NeighborState& neighbor) {
    return request.config.dynamics_limits_2d.a_max_dec *
        (neighbor.motion_state == NeighborMotionState::Arrived ? 1.0 : 2.0);
}

double DynamicPredictionHorizon(
    const CollisionAvoidanceRequest& request,
    const NeighborState& neighbor,
    const Vector2D& self_velocity) {
    const double closing_speed = ClosingSpeed(request, neighbor, self_velocity);
    const double reaction_time = request.dt + request.communication_delay;
    const double braking_time = closing_speed /
        AvailableRelativeDeceleration(request, neighbor);
    const double lateral_clearance_time = std::sqrt(
        2.0 * PlanningSafetyDistance(request) /
        request.config.dynamics_limits_2d.a_max_lat);
    const double boundary_crossing_time = closing_speed > kEpsilon
        ? PlanningSafetyDistance(request) / closing_speed : 0.0;
    return std::max(request.dt,
        reaction_time + braking_time +
        std::max(boundary_crossing_time, lateral_clearance_time));
}

double DynamicWarningDistance(
    const CollisionAvoidanceRequest& request,
    const NeighborState& neighbor,
    const Vector2D& self_velocity) {
    const double closing_speed = ClosingSpeed(request, neighbor, self_velocity);
    const double reaction_time = request.dt + request.communication_delay;

    const double reaction_allowance = closing_speed * reaction_time;
    const double braking_allowance =
        closing_speed * closing_speed /
            (2.0 * AvailableRelativeDeceleration(request, neighbor));
    const double lateral_clearance_time = std::sqrt(
        2.0 * PlanningSafetyDistance(request) /
        request.config.dynamics_limits_2d.a_max_lat);
    const double lateral_allowance = closing_speed * lateral_clearance_time;
    return PlanningSafetyDistance(request) + reaction_allowance +
        braking_allowance + lateral_allowance;
}

Vector2D PairwiseSideDirection(
    const CollisionAvoidanceRequest& request,
    const NeighborState& neighbor) {
    // Both drones construct the same low-ID -> high-ID axis, then take opposite
    // sides. This breaks perfect symmetry without non-repeatable random numbers.
    const bool self_is_lower = request.drone_id < neighbor.drone_id;
    // Use the canonical relative travel direction instead of the rotating
    // line-of-sight. This keeps the chosen passing side stable while two drones
    // go around each other and prevents orbit-like feedback.
    const Vector2D ordered_motion = self_is_lower
        ? Subtract(request.desired_velocity, EffectiveNeighborVelocity(neighbor))
        : Subtract(EffectiveNeighborVelocity(neighbor), request.desired_velocity);
    const Vector2D ordered_position = self_is_lower
        ? Subtract(neighbor.position, request.current_position)
        : Subtract(request.current_position, neighbor.position);
    const Vector2D ordered_axis = Length(ordered_motion) > kEpsilon
        ? ordered_motion : ordered_position;
    const Vector2D normalized_axis = NormalizeOr(ordered_axis, {1.0, 0.0});
    Vector2D perpendicular{-normalized_axis.y, normalized_axis.x};
    if (!self_is_lower) {
        perpendicular = Scale(perpendicular, -1.0);
    }
    return perpendicular;
}

double AvoidanceResponsibility(
    const CollisionAvoidanceRequest& request,
    const NeighborState& neighbor) {
    const double self_speed = Length(request.desired_velocity);
    if (neighbor.motion_state == NeighborMotionState::Arrived) {
        return 1.0;
    }
    const double neighbor_speed = Length(EffectiveNeighborVelocity(neighbor));
    if (neighbor_speed <= request.stopped_speed_tolerance) {
        return 1.0;
    }
    const double total = self_speed + neighbor_speed;
    if (total <= kEpsilon) {
        return 0.5;
    }
    return std::clamp(self_speed / total, 0.25, 0.75);
}

Vector2D ClampToReachableVelocity(
    const CollisionAvoidanceRequest& request,
    const Vector2D& candidate) {
    const Vector2D current = request.current_velocity;
    const Vector2D requested_acceleration = Scale(
        Subtract(candidate, current), 1.0 / request.dt);
    const double current_speed = Length(current);
    Vector2D limited_acceleration = requested_acceleration;

    if (current_speed > request.stopped_speed_tolerance) {
        const Vector2D tangent = Scale(current, 1.0 / current_speed);
        const Vector2D normal{-tangent.y, tangent.x};
        const double longitudinal = std::clamp(
            Dot(requested_acceleration, tangent),
            -request.config.dynamics_limits_2d.a_max_dec,
            request.config.dynamics_limits_2d.a_max_acc);
        const double lateral = std::clamp(
            Dot(requested_acceleration, normal),
            -request.config.dynamics_limits_2d.a_max_lat,
            request.config.dynamics_limits_2d.a_max_lat);
        limited_acceleration = Add(
            Scale(tangent, longitudinal), Scale(normal, lateral));
    } else {
        limited_acceleration = ClampMagnitude(
            requested_acceleration, request.config.dynamics_limits_2d.a_max_acc);
    }

    return ClampMagnitude(Add(current,
        Scale(limited_acceleration, request.dt)), request.config.dynamics_limits_2d.v_max);
}

double MaximumCorrectionSpeed(const CollisionAvoidanceRequest& request) {
    return std::max(request.config.dynamics_limits_2d.a_max_lat,
        request.config.dynamics_limits_2d.a_max_dec) * request.dt;
}

} // namespace

CollisionAvoidanceResult ComputeSafeVelocity(
    const CollisionAvoidanceRequest& request) {
    CollisionAvoidanceResult result;
    result.safe_velocity = {0.0, 0.0};
    result.predicted_min_distance = std::numeric_limits<double>::infinity();

    if (!IsValidRequest(request)) {
        result.status = AvoidanceStatus::InvalidInput;
        result.predicted_min_distance = 0.0;
        return result;
    }

    const double distance_to_target = Length(
        Subtract(request.target_position, request.current_position));
    if (distance_to_target <= request.arrival_tolerance &&
        Length(request.desired_velocity) <= request.stopped_speed_tolerance) {
        result.status = AvoidanceStatus::Arrived;
        result.predicted_min_distance = request.neighbors.empty()
            ? std::numeric_limits<double>::infinity()
            : result.predicted_min_distance;
        return result;
    }

    const Vector2D desired_velocity = ClampToReachableVelocity(request,
        ClampMagnitude(request.desired_velocity, request.config.dynamics_limits_2d.v_max));
    Vector2D accumulated_correction{0.0, 0.0};
    double total_risk = 0.0;
    bool has_arrived_blocker = false;
    double arrived_blocker_risk = 0.0;
    Vector2D arrived_escape_side{0.0, 0.0};

    for (const NeighborState& neighbor : request.neighbors) {
        const Vector2D neighbor_velocity = EffectiveNeighborVelocity(neighbor);
        const double prediction_horizon = DynamicPredictionHorizon(
            request, neighbor, desired_velocity);
        const detail::ClosestApproach approach = detail::PredictClosestApproach(
            request.current_position, desired_velocity,
            neighbor.position, neighbor_velocity,
            prediction_horizon);
        result.predicted_min_distance = std::min(
            result.predicted_min_distance, approach.distance);

        const double warning_distance = DynamicWarningDistance(
            request, neighbor, desired_velocity);
        const double safe_distance = PlanningSafetyDistance(request);
        const Vector2D current_separation = Subtract(
            neighbor.position, request.current_position);
        const Vector2D line_of_sight = NormalizeOr(current_separation, {1.0, 0.0});
        const double closing_speed = Dot(
            Subtract(desired_velocity, neighbor_velocity), line_of_sight);
        if (approach.time <= kEpsilon &&
            Length(current_separation) > BaseSafetyDistance(request) &&
            closing_speed <= 0.0) {
            continue;
        }
        if (approach.distance >= warning_distance) {
            continue;
        }

        const double risk = std::clamp(
            (warning_distance - approach.distance) /
                std::max(warning_distance - safe_distance, kEpsilon),
            0.0, 1.0);
        const double responsibility = AvoidanceResponsibility(request, neighbor);

        // Project the requested closing speed onto a braking-feasible bound.
        // c*t + c^2/(2a) <= clearance; solving the quadratic gives the largest
        // closing speed that can still stop outside the combined circular body.
        const double current_distance = Length(current_separation);
        const double clearance = std::max(0.0,
            current_distance - safe_distance);
        const double reaction_time = request.dt +
            request.communication_delay;
        const double relative_deceleration =
            AvailableRelativeDeceleration(request, neighbor);
        const double maximum_safe_closing_speed = std::max(0.0,
            -relative_deceleration * reaction_time + std::sqrt(
                relative_deceleration * relative_deceleration *
                    reaction_time * reaction_time +
                2.0 * relative_deceleration * clearance));
        const double excess_closing_speed = std::max(0.0,
            closing_speed - maximum_safe_closing_speed);

        const Vector2D self_at_closest = Add(
            request.current_position, Scale(desired_velocity, approach.time));
        const Vector2D neighbor_at_closest = Add(
            neighbor.position, Scale(neighbor_velocity, approach.time));
        const Vector2D away = NormalizeOr(
            Subtract(self_at_closest, neighbor_at_closest),
            Scale(PairwiseSideDirection(request, neighbor), 0.25));
        const Vector2D side = PairwiseSideDirection(request, neighbor);

        if (neighbor.motion_state == NeighborMotionState::Arrived &&
            closing_speed > 0.0 && risk > arrived_blocker_risk) {
            has_arrived_blocker = true;
            arrived_blocker_risk = risk;
            arrived_escape_side = side;
        }

        const double correction_capacity = MaximumCorrectionSpeed(request);
        const double strength = risk * responsibility * correction_capacity;

        accumulated_correction = Add(accumulated_correction,
            Add(Add(Scale(away, strength), Scale(side, strength)),
                Scale(line_of_sight,
                    -excess_closing_speed * responsibility)));
        total_risk += risk;
    }

    if (total_risk <= kEpsilon) {
        result.safe_velocity = desired_velocity;
        result.modified = Length(Subtract(desired_velocity,
            request.desired_velocity)) > kEpsilon;
        result.status = result.modified
            ? AvoidanceStatus::Avoiding : AvoidanceStatus::Safe;
        return result;
    }

    const double maximum_correction = MaximumCorrectionSpeed(request);
    accumulated_correction = ClampMagnitude(
        accumulated_correction, maximum_correction);
    Vector2D candidate_velocity = Add(desired_velocity,
        accumulated_correction);
    if (has_arrived_blocker) {
        // A pure radial correction can exactly cancel the goal velocity and
        // leave a moving drone parked against an arrived one. Preserve a
        // deterministic tangential escape component until the blocker is passed.
        const double desired_speed = Length(desired_velocity);
        const double minimum_escape_speed = std::min(request.config.dynamics_limits_2d.v_max,
            std::max(request.config.dynamics_limits_2d.a_max_acc * request.dt,
                std::min(desired_speed,
                    request.config.dynamics_limits_2d.a_max_lat)));
        const double existing_escape_speed = Dot(candidate_velocity,
            arrived_escape_side);
        if (existing_escape_speed < minimum_escape_speed) {
            candidate_velocity = Add(candidate_velocity,
                Scale(arrived_escape_side,
                    minimum_escape_speed - existing_escape_speed));
        }
    }
    result.safe_velocity = ClampToReachableVelocity(request,
        candidate_velocity);
    result.modified = Length(Subtract(result.safe_velocity,
        request.desired_velocity)) > kEpsilon;
    result.status = AvoidanceStatus::Avoiding;
    return result;
}

const char* AvoidanceStatusMessage(AvoidanceStatus status) {
    switch (status) {
    case AvoidanceStatus::Safe:
        return "safe";
    case AvoidanceStatus::Avoiding:
        return "avoiding";
    case AvoidanceStatus::Arrived:
        return "arrived";
    case AvoidanceStatus::InvalidInput:
        return "invalid input";
    }
    return "unknown";
}

} // namespace SwarmAvoidance
