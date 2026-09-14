#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "CollisionAvoidance.h"
#include "CollisionPrediction.h"

using SwarmAvoidance::AvoidanceStatus;
using SwarmAvoidance::CollisionAvoidanceRequest;
using SwarmAvoidance::CollisionAvoidanceResult;
using SwarmAvoidance::NeighborState;
using SwarmAvoidance::NeighborMotionState;
using SwarmAvoidance::Vector2D;

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "[FAIL] " << message << '\n';
    }
}

double Length(const Vector2D& value) {
    return std::sqrt(value.x * value.x + value.y * value.y);
}

bool Near(double lhs, double rhs, double tolerance = 1e-9) {
    return std::fabs(lhs - rhs) <= tolerance;
}

struct SimDrone {
    unsigned long id;
    Vector2D position;
    Vector2D velocity;
    Vector2D target;
    bool fixed = false;
};

struct SimulationResult {
    double minimum_distance = std::numeric_limits<double>::infinity();
    double maximum_target_error = 0.0;
};

Vector2D DirectionToTarget(const SimDrone& drone, double cruise_speed) {
    const Vector2D delta{
        drone.target.x - drone.position.x,
        drone.target.y - drone.position.y
    };
    const double distance = Length(delta);
    if (drone.fixed || distance <= 0.03) {
        return {0.0, 0.0};
    }
    const double speed = std::min(cruise_speed, distance);
    return {delta.x / distance * speed, delta.y / distance * speed};
}

Vector2D ApplySimulatedDynamics(
    const Vector2D& current,
    const Vector2D& desired,
    double max_delta) {
    Vector2D delta{desired.x - current.x, desired.y - current.y};
    const double length = Length(delta);
    if (length > max_delta && length > 1e-12) {
        delta.x *= max_delta / length;
        delta.y *= max_delta / length;
    }
    return {current.x + delta.x, current.y + delta.y};
}

SimulationResult Simulate(
    std::vector<SimDrone> drones,
    int steps = 300,
    double cruise_speed = 1.3) {
    constexpr double dt = 0.1;
    constexpr double safe_distance = 0.8;
    SimulationResult summary;

    for (int step = 0; step < steps; ++step) {
        std::vector<Vector2D> next_velocities(drones.size());
        for (std::size_t i = 0; i < drones.size(); ++i) {
            if (drones[i].fixed) {
                next_velocities[i] = {0.0, 0.0};
                continue;
            }
            CollisionAvoidanceRequest request;
            request.drone_id = drones[i].id;
            request.current_position = drones[i].position;
            request.current_velocity = drones[i].velocity;
            request.desired_velocity = DirectionToTarget(drones[i], cruise_speed);
            request.target_position = drones[i].target;
            request.dt = dt;
            request.config.safe_distance = safe_distance;
            request.config.warning_distance = 1.3;
            request.config.prediction_horizon = 2.5;
            request.config.max_speed = 3.0;
            request.config.max_acceleration = 2.0;
            request.config.max_deceleration = 3.0;
            request.config.max_lateral_acceleration = 2.5;
            request.config.arrival_tolerance = 0.05;

            for (std::size_t j = 0; j < drones.size(); ++j) {
                if (i != j) {
                    request.neighbors.push_back({drones[j].id,
                        drones[j].position, drones[j].velocity});
                }
            }
            const auto filtered = SwarmAvoidance::ComputeSafeVelocity(request);
            next_velocities[i] = ApplySimulatedDynamics(
                drones[i].velocity, filtered.safe_velocity,
                request.config.max_lateral_acceleration * dt);
        }

        for (std::size_t i = 0; i < drones.size(); ++i) {
            drones[i].velocity = next_velocities[i];
            drones[i].position.x += drones[i].velocity.x * dt;
            drones[i].position.y += drones[i].velocity.y * dt;
        }
        for (std::size_t i = 0; i < drones.size(); ++i) {
            for (std::size_t j = i + 1; j < drones.size(); ++j) {
                summary.minimum_distance = std::min(summary.minimum_distance,
                    Length({drones[i].position.x - drones[j].position.x,
                        drones[i].position.y - drones[j].position.y}));
            }
        }
    }

    for (const SimDrone& drone : drones) {
        if (!drone.fixed) {
            summary.maximum_target_error = std::max(summary.maximum_target_error,
                Length({drone.target.x - drone.position.x,
                    drone.target.y - drone.position.y}));
        }
    }
    return summary;
}

CollisionAvoidanceRequest BaseRequest() {
    CollisionAvoidanceRequest request;
    request.drone_id = 1;
    request.current_position = {-5.0, 0.0};
    request.current_velocity = {1.0, 0.0};
    request.desired_velocity = {1.0, 0.0};
    request.target_position = {5.0, 0.0};
    request.dt = 0.1;
    request.config.safe_distance = 1.0;
    request.config.warning_distance = 1.5;
    request.config.prediction_horizon = 3.0;
    request.config.max_speed = 5.0;
    request.config.max_acceleration = 2.0;
    request.config.max_deceleration = 3.0;
    request.config.max_lateral_acceleration = 2.5;
    return request;
}

void TestClosestApproach() {
    const auto result = SwarmAvoidance::detail::PredictClosestApproach(
        {-2.0, 0.0}, {1.0, 0.0},
        {2.0, 0.0}, {-1.0, 0.0}, 5.0);
    Check(Near(result.time, 2.0), "head-on closest time should be 2 seconds");
    Check(Near(result.distance, 0.0), "head-on closest distance should be zero");
}

void TestNoNeighborPassThrough() {
    const CollisionAvoidanceRequest request = BaseRequest();
    const CollisionAvoidanceResult result =
        SwarmAvoidance::ComputeSafeVelocity(request);
    Check(result.status == AvoidanceStatus::Safe, "no-neighbor request should be safe");
    Check(!result.modified, "no-neighbor request should not be modified");
    Check(Near(result.safe_velocity.x, request.desired_velocity.x) &&
        Near(result.safe_velocity.y, request.desired_velocity.y),
        "safe request should preserve C's desired velocity");
}

void TestDistantNeighborPassThrough() {
    CollisionAvoidanceRequest request = BaseRequest();
    request.neighbors.push_back({2, {100.0, 100.0}, {0.0, 0.0}});
    const auto result = SwarmAvoidance::ComputeSafeVelocity(request);
    Check(result.status == AvoidanceStatus::Safe,
        "distant neighbor should not trigger avoidance");
    Check(!result.modified, "distant neighbor should preserve desired velocity");
}

void TestNearbySeparatingNeighborPassThrough() {
    CollisionAvoidanceRequest request = BaseRequest();
    request.current_position = {0.0, 0.0};
    request.current_velocity = {-1.0, 0.0};
    request.desired_velocity = {-1.0, 0.0};
    request.target_position = {-5.0, 0.0};
    request.neighbors.push_back({2, {1.2, 0.0}, {1.0, 0.0}});
    const auto result = SwarmAvoidance::ComputeSafeVelocity(request);
    Check(result.status == AvoidanceStatus::Safe && !result.modified,
        "nearby drones moving apart should not keep avoiding");
}

void TestHeadOnUsesOppositeSides() {
    CollisionAvoidanceRequest low = BaseRequest();
    low.current_position = {-2.0, 0.0};
    low.target_position = {3.0, 0.0};
    low.neighbors.push_back({2, {2.0, 0.0}, {-1.0, 0.0}});

    CollisionAvoidanceRequest high = BaseRequest();
    high.drone_id = 2;
    high.current_position = {2.0, 0.0};
    high.current_velocity = {-1.0, 0.0};
    high.desired_velocity = {-1.0, 0.0};
    high.target_position = {-3.0, 0.0};
    high.neighbors.push_back({1, {-2.0, 0.0}, {1.0, 0.0}});

    const auto low_result = SwarmAvoidance::ComputeSafeVelocity(low);
    const auto high_result = SwarmAvoidance::ComputeSafeVelocity(high);
    Check(low_result.status == AvoidanceStatus::Avoiding &&
        high_result.status == AvoidanceStatus::Avoiding,
        "head-on drones should both detect risk");
    Check(low_result.safe_velocity.y * high_result.safe_velocity.y < 0.0,
        "head-on drones should choose opposite sides");
}

void TestCrossingTrajectories() {
    CollisionAvoidanceRequest request = BaseRequest();
    request.current_position = {-1.0, 0.0};
    request.target_position = {2.0, 0.0};
    request.neighbors.push_back({2, {0.0, -1.0}, {0.0, 1.0}});
    const auto result = SwarmAvoidance::ComputeSafeVelocity(request);
    Check(result.status == AvoidanceStatus::Avoiding,
        "crossing trajectories should trigger avoidance");
    Check(result.predicted_min_distance < request.config.safe_distance,
        "crossing prediction should report unsafe closest distance");
}

void TestArrivedDroneStaysStill() {
    CollisionAvoidanceRequest request = BaseRequest();
    request.current_position = {0.0, 0.0};
    request.current_velocity = {0.0, 0.0};
    request.desired_velocity = {0.0, 0.0};
    request.target_position = {0.01, 0.0};
    request.neighbors.push_back({2, {2.0, 0.0}, {-1.0, 0.0}});
    const auto result = SwarmAvoidance::ComputeSafeVelocity(request);
    Check(result.status == AvoidanceStatus::Arrived,
        "arrived drone should report Arrived");
    Check(Near(Length(result.safe_velocity), 0.0),
        "arrived drone should not actively move");
}

void TestStationaryNeighborGetsMoreResponsibilityFromMover() {
    CollisionAvoidanceRequest request = BaseRequest();
    request.current_position = {-2.0, 0.0};
    request.target_position = {2.0, 0.0};
    request.neighbors.push_back({2, {0.0, 0.0}, {0.0, 0.0}});
    const auto result = SwarmAvoidance::ComputeSafeVelocity(request);
    Check(result.status == AvoidanceStatus::Avoiding,
        "moving drone should avoid a stationary neighbor");
    Check(std::fabs(result.safe_velocity.y) > 0.0,
        "stationary-neighbor case should include lateral avoidance");
}

void TestArrivedNeighborIsTreatedAsFixedObstacle() {
    CollisionAvoidanceRequest request = BaseRequest();
    request.current_position = {-2.0, 0.0};
    request.target_position = {2.0, 0.0};

    NeighborState arrived;
    arrived.drone_id = 2;
    arrived.position = {0.0, 0.0};
    // Deliberately stale telemetry: Arrived must override this velocity.
    arrived.velocity = {4.0, 0.0};
    arrived.motion_state = NeighborMotionState::Arrived;
    request.neighbors.push_back(arrived);

    const auto result = SwarmAvoidance::ComputeSafeVelocity(request);
    Check(result.status == AvoidanceStatus::Avoiding,
        "arrived neighbor should be treated as a fixed obstacle");
    Check(std::fabs(result.safe_velocity.y) > 0.0,
        "mover should steer around an arrived neighbor");
}

void TestLowSpeedCorrectionRemainsSmall() {
    CollisionAvoidanceRequest request = BaseRequest();
    request.current_position = {-0.2, 0.0};
    request.current_velocity = {0.1, 0.0};
    request.desired_velocity = {0.1, 0.0};
    request.target_position = {0.2, 0.0};
    request.neighbors.push_back({2, {0.7, 0.0}, {-0.6, 0.0}});
    const auto result = SwarmAvoidance::ComputeSafeVelocity(request);
    const Vector2D correction{
        result.safe_velocity.x - request.desired_velocity.x,
        result.safe_velocity.y - request.desired_velocity.y
    };
    Check(result.status == AvoidanceStatus::Avoiding,
        "low-speed conflict should still be detected");
    Check(Length(correction) <=
        request.config.max_lateral_acceleration * request.dt + 1e-9,
        "low-speed drone must not receive a disproportionate correction");
}

void TestMultipleNeighborsRemainBounded() {
    CollisionAvoidanceRequest request = BaseRequest();
    request.current_position = {0.0, 0.0};
    request.target_position = {10.0, 0.0};
    request.neighbors = {
        {2, {2.0, 0.0}, {-1.0, 0.0}},
        {3, {2.0, 0.5}, {-1.0, 0.0}},
        {4, {2.0, -0.5}, {-1.0, 0.0}}
    };
    const auto result = SwarmAvoidance::ComputeSafeVelocity(request);
    Check(result.status == AvoidanceStatus::Avoiding,
        "multiple threats should trigger avoidance");
    Check(Length(result.safe_velocity) <= request.config.max_speed + 1e-9,
        "multiple-neighbor correction must stay within max speed");
}

void TestInvalidInput() {
    CollisionAvoidanceRequest request = BaseRequest();
    request.dt = 0.0;
    Check(SwarmAvoidance::ComputeSafeVelocity(request).status ==
        AvoidanceStatus::InvalidInput, "dt=0 should be rejected");

    request = BaseRequest();
    request.current_position.x = std::numeric_limits<double>::quiet_NaN();
    Check(SwarmAvoidance::ComputeSafeVelocity(request).status ==
        AvoidanceStatus::InvalidInput, "NaN position should be rejected");

    request = BaseRequest();
    request.neighbors.push_back({request.drone_id, {0.0, 0.0}, {0.0, 0.0}});
    Check(SwarmAvoidance::ComputeSafeVelocity(request).status ==
        AvoidanceStatus::InvalidInput, "self must not appear in neighbor list");
}

void TestDeterministicAndSpeedLimited() {
    CollisionAvoidanceRequest request = BaseRequest();
    request.desired_velocity = {20.0, 0.0};
    request.neighbors.push_back({2, {3.0, 0.0}, {-1.0, 0.0}});
    const auto first = SwarmAvoidance::ComputeSafeVelocity(request);
    const auto second = SwarmAvoidance::ComputeSafeVelocity(request);
    Check(Near(first.safe_velocity.x, second.safe_velocity.x) &&
        Near(first.safe_velocity.y, second.safe_velocity.y),
        "same input should produce exactly the same velocity");
    Check(Length(first.safe_velocity) <= request.config.max_speed + 1e-9,
        "output velocity must be limited");
}

void TestDifferentControlPeriods() {
    CollisionAvoidanceRequest fast = BaseRequest();
    fast.current_position = {-2.0, 0.0};
    fast.neighbors.push_back({2, {2.0, 0.0}, {-1.0, 0.0}});
    CollisionAvoidanceRequest slow = fast;
    slow.dt = 0.2;
    const auto fast_result = SwarmAvoidance::ComputeSafeVelocity(fast);
    const auto slow_result = SwarmAvoidance::ComputeSafeVelocity(slow);
    Check(fast_result.status == AvoidanceStatus::Avoiding &&
        slow_result.status == AvoidanceStatus::Avoiding,
        "algorithm should support different positive dt values");
}

void TestDynamicHeadOnScenario() {
    const SimulationResult result = Simulate({
        {1, {-6.0, 0.0}, {0.0, 0.0}, {6.0, 0.0}},
        {2, {6.0, 0.0}, {0.0, 0.0}, {-6.0, 0.0}}
    });
    Check(result.minimum_distance >= 0.8,
        "dynamic head-on scenario must preserve safe distance");
    Check(result.maximum_target_error <= 0.1,
        "dynamic head-on scenario should still reach both targets");
}

void TestDynamicCrossingScenario() {
    const SimulationResult result = Simulate({
        {1, {-5.0, 0.0}, {0.0, 0.0}, {5.0, 0.0}},
        {2, {0.0, -5.0}, {0.0, 0.0}, {0.0, 5.0}}
    });
    Check(result.minimum_distance >= 0.8,
        "dynamic crossing scenario must preserve safe distance");
    Check(result.maximum_target_error <= 0.1,
        "dynamic crossing scenario should still reach both targets");
}

void TestDynamicStationaryNeighborScenario() {
    const SimulationResult result = Simulate({
        {1, {-5.0, 0.0}, {0.0, 0.0}, {5.0, 0.0}},
        {2, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, true}
    });
    Check(result.minimum_distance >= 0.8,
        "moving drone must avoid an arrived stationary drone");
    Check(result.maximum_target_error <= 0.1,
        "moving drone should recover its route after avoiding a stationary drone");
}

void TestDynamicThreeWayScenario() {
    const SimulationResult result = Simulate({
        {1, {-6.0, 0.0}, {0.0, 0.0}, {6.0, 0.0}},
        {2, {6.0, 0.0}, {0.0, 0.0}, {-6.0, 0.0}},
        {3, {0.0, -4.0}, {0.0, 0.0}, {0.0, 4.0}}
    });
    Check(result.minimum_distance >= 0.8,
        "dynamic three-way scenario must preserve safe distance");
    Check(result.maximum_target_error <= 0.1,
        "dynamic three-way scenario should reach all targets");
}

} // namespace

int main() {
    TestClosestApproach();
    TestNoNeighborPassThrough();
    TestDistantNeighborPassThrough();
    TestNearbySeparatingNeighborPassThrough();
    TestHeadOnUsesOppositeSides();
    TestCrossingTrajectories();
    TestArrivedDroneStaysStill();
    TestStationaryNeighborGetsMoreResponsibilityFromMover();
    TestArrivedNeighborIsTreatedAsFixedObstacle();
    TestLowSpeedCorrectionRemainsSmall();
    TestMultipleNeighborsRemainBounded();
    TestInvalidInput();
    TestDeterministicAndSpeedLimited();
    TestDifferentControlPeriods();
    TestDynamicHeadOnScenario();
    TestDynamicCrossingScenario();
    TestDynamicStationaryNeighborScenario();
    TestDynamicThreeWayScenario();

    if (failures == 0) {
        std::cout << "All collision-avoidance tests passed.\n";
        return 0;
    }
    std::cerr << failures << " collision-avoidance test(s) failed.\n";
    return 1;
}
