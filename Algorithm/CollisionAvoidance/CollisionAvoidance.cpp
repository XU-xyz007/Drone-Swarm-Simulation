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

bool IsValidConfig(const CollisionAvoidanceConfig& config) {
    return std::isfinite(config.safe_distance) && config.safe_distance > 0.0 &&
        std::isfinite(config.warning_distance) &&
        config.warning_distance >= config.safe_distance &&
        std::isfinite(config.prediction_horizon) && config.prediction_horizon > 0.0 &&
        std::isfinite(config.max_speed) && config.max_speed > 0.0 &&
        std::isfinite(config.max_acceleration) && config.max_acceleration > 0.0 &&
        std::isfinite(config.max_deceleration) && config.max_deceleration > 0.0 &&
        std::isfinite(config.max_lateral_acceleration) &&
        config.max_lateral_acceleration > 0.0 &&
        std::isfinite(config.arrival_tolerance) && config.arrival_tolerance >= 0.0 &&
        std::isfinite(config.stopped_speed_tolerance) &&
        config.stopped_speed_tolerance >= 0.0 &&
        std::isfinite(config.avoidance_gain) && config.avoidance_gain > 0.0;
}

bool IsValidRequest(const CollisionAvoidanceRequest& request) {
    if (!std::isfinite(request.dt) || request.dt <= 0.0 ||
        !IsFinite(request.current_position) ||
        !IsFinite(request.current_velocity) ||
        !IsFinite(request.desired_velocity) ||
        !IsFinite(request.target_position) ||
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

double DynamicWarningDistance(
    const CollisionAvoidanceRequest& request,
    const NeighborState& neighbor) {
    const Vector2D separation = Subtract(neighbor.position, request.current_position);
    const Vector2D line_of_sight = NormalizeOr(separation, {1.0, 0.0});
    const Vector2D relative_velocity = Subtract(
        request.desired_velocity, EffectiveNeighborVelocity(neighbor));
    const double closing_speed = std::max(0.0, Dot(relative_velocity, line_of_sight));

    // Reaction allowance grows with the actual call period. The braking allowance
    // is deliberately bounded so that a distant fast neighbor does not dominate.
    const double reaction_allowance = closing_speed * request.dt;
    const double braking_allowance = std::min(
        closing_speed * closing_speed /
            (2.0 * request.config.max_deceleration),
        request.config.warning_distance);
    const double dynamic_distance = request.config.safe_distance +
        reaction_allowance + 0.5 * braking_allowance;
    const double upper_bound = request.config.warning_distance +
        request.config.max_speed * request.config.prediction_horizon * 0.5;
    return std::clamp(dynamic_distance,
        request.config.warning_distance, upper_bound);
}

Vector2D PairwiseSideDirection(
    const CollisionAvoidanceRequest& request,
    const NeighborState& neighbor) {
    // Both drones construct the same low-ID -> high-ID axis, then take opposite
    // sides. This breaks perfect symmetry without non-repeatable random numbers.
    const bool self_is_lower = request.drone_id < neighbor.drone_id;
    const Vector2D ordered_axis = self_is_lower
        ? Subtract(neighbor.position, request.current_position)
        : Subtract(request.current_position, neighbor.position);
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
    if (neighbor_speed <= request.config.stopped_speed_tolerance) {
        return 1.0;
    }
    const double total = self_speed + neighbor_speed;
    if (total <= kEpsilon) {
        return 0.5;
    }
    return std::clamp(self_speed / total, 0.25, 0.75);
}

double MaximumCorrectionSpeed(const CollisionAvoidanceRequest& request) {
    const double intended_speed = Length(request.desired_velocity);
    const double low_speed_limit = std::max(
        request.config.max_lateral_acceleration * request.dt,
        intended_speed * 1.25);
    const double physical_limit = std::max(
        request.config.max_lateral_acceleration,
        request.config.max_deceleration) * request.config.prediction_horizon;
    return std::min(request.config.max_speed,
        std::min(low_speed_limit, physical_limit));
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
    if (distance_to_target <= request.config.arrival_tolerance &&
        Length(request.desired_velocity) <= request.config.stopped_speed_tolerance) {
        result.status = AvoidanceStatus::Arrived;
        result.predicted_min_distance = request.neighbors.empty()
            ? std::numeric_limits<double>::infinity()
            : result.predicted_min_distance;
        return result;
    }

    const Vector2D desired_velocity = ClampMagnitude(
        request.desired_velocity, request.config.max_speed);
    Vector2D accumulated_correction{0.0, 0.0};
    double total_risk = 0.0;

    for (const NeighborState& neighbor : request.neighbors) {
        const Vector2D neighbor_velocity = EffectiveNeighborVelocity(neighbor);
        const detail::ClosestApproach approach = detail::PredictClosestApproach(
            request.current_position, desired_velocity,
            neighbor.position, neighbor_velocity,
            request.config.prediction_horizon);
        result.predicted_min_distance = std::min(
            result.predicted_min_distance, approach.distance);

        const double warning_distance = DynamicWarningDistance(request, neighbor);
        const Vector2D current_separation = Subtract(
            neighbor.position, request.current_position);
        const Vector2D line_of_sight = NormalizeOr(current_separation, {1.0, 0.0});
        const double closing_speed = Dot(
            Subtract(desired_velocity, neighbor_velocity), line_of_sight);
        if (approach.time <= kEpsilon &&
            Length(current_separation) > request.config.safe_distance &&
            closing_speed <= 0.0) {
            continue;
        }
        if (approach.distance >= warning_distance) {
            continue;
        }

        const double risk = std::clamp(
            (warning_distance - approach.distance) /
                std::max(warning_distance - request.config.safe_distance, kEpsilon),
            0.0, 1.0);
        const double responsibility = AvoidanceResponsibility(request, neighbor);

        const Vector2D self_at_closest = Add(
            request.current_position, Scale(desired_velocity, approach.time));
        const Vector2D neighbor_at_closest = Add(
            neighbor.position, Scale(neighbor_velocity, approach.time));
        const Vector2D away = NormalizeOr(
            Subtract(self_at_closest, neighbor_at_closest),
            Scale(PairwiseSideDirection(request, neighbor), 0.25));
        const Vector2D side = PairwiseSideDirection(request, neighbor);

        const double correction_capacity = MaximumCorrectionSpeed(request);
        const double strength = request.config.avoidance_gain *
            risk * responsibility * correction_capacity;

        accumulated_correction = Add(accumulated_correction,
            Add(Scale(away, strength * 0.55),
                Scale(side, strength * 0.75)));
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
    result.safe_velocity = ClampMagnitude(
        Add(desired_velocity, accumulated_correction),
        request.config.max_speed);
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
