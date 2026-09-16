#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "CollisionAvoidance.h"

using SwarmAvoidance::CollisionAvoidanceRequest;
using SwarmAvoidance::NeighborState;
using SwarmAvoidance::Vector2D;

namespace {

struct DroneSim {
    unsigned long id;
    Vector2D position;
    Vector2D velocity;
    Vector2D target;
    std::vector<Vector2D> trail;
};

Vector2D Subtract(const Vector2D& lhs, const Vector2D& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

double Length(const Vector2D& value) {
    return std::sqrt(value.x * value.x + value.y * value.y);
}

Vector2D DesiredVelocity(const DroneSim& drone, double speed) {
    const Vector2D delta = Subtract(drone.target, drone.position);
    const double distance = Length(delta);
    if (distance < 0.05) {
        return {0.0, 0.0};
    }
    const double commanded_speed = std::min(speed, distance);
    return {delta.x / distance * commanded_speed,
        delta.y / distance * commanded_speed};
}

Vector2D ApplyDynamics(
    const Vector2D& current,
    const Vector2D& target,
    double maximum_acceleration,
    double dt) {
    Vector2D delta{target.x - current.x, target.y - current.y};
    const double delta_length = Length(delta);
    const double maximum_delta = maximum_acceleration * dt;
    if (delta_length > maximum_delta && delta_length > 1e-9) {
        delta.x *= maximum_delta / delta_length;
        delta.y *= maximum_delta / delta_length;
    }
    return {current.x + delta.x, current.y + delta.y};
}

double RunScenario(std::vector<DroneSim>& drones, const std::string& csv_path) {
    const double dt = 0.1;
    double minimum_distance = 1e9;
    std::ofstream csv(csv_path);
    csv << "time,drone_id,x,y,vx,vy,avoiding\n";

    for (int step = 0; step < 240; ++step) {
        std::vector<Vector2D> next_velocities(drones.size());
        for (std::size_t i = 0; i < drones.size(); ++i) {
            CollisionAvoidanceRequest request;
            request.drone_id = drones[i].id;
            request.current_position = drones[i].position;
            request.current_velocity = drones[i].velocity;
            request.desired_velocity = DesiredVelocity(drones[i], 1.3);
            request.target_position = drones[i].target;
            request.dt = dt;
            request.config.avoidance_params.drone_radius = 0.5;
            request.config.avoidance_params.safety_margin = 1.0;
            request.config.dynamics_limits_2d.v_max = 5.0;
            request.config.dynamics_limits_2d.a_max_acc = 3.0;
            request.config.dynamics_limits_2d.a_max_dec = 5.0;
            request.config.dynamics_limits_2d.a_max_lat = 4.0;

            for (std::size_t j = 0; j < drones.size(); ++j) {
                if (i != j) {
                    request.neighbors.push_back({drones[j].id,
                        drones[j].position, drones[j].velocity});
                }
            }
            const auto result = SwarmAvoidance::ComputeSafeVelocity(request);
            next_velocities[i] = ApplyDynamics(drones[i].velocity,
                result.safe_velocity, request.config.dynamics_limits_2d.a_max_lat, dt);
            csv << std::fixed << std::setprecision(4)
                << step * dt << ',' << drones[i].id << ','
                << drones[i].position.x << ',' << drones[i].position.y << ','
                << next_velocities[i].x << ',' << next_velocities[i].y << ','
                << (result.modified ? 1 : 0) << '\n';
        }

        for (std::size_t i = 0; i < drones.size(); ++i) {
            drones[i].velocity = next_velocities[i];
            drones[i].position.x += drones[i].velocity.x * dt;
            drones[i].position.y += drones[i].velocity.y * dt;
            drones[i].trail.push_back(drones[i].position);
        }
        for (std::size_t i = 0; i < drones.size(); ++i) {
            for (std::size_t j = i + 1; j < drones.size(); ++j) {
                minimum_distance = std::min(minimum_distance,
                    Length(Subtract(drones[i].position, drones[j].position)));
            }
        }
    }
    return minimum_distance;
}

void WriteSvg(const std::vector<DroneSim>& drones, const std::string& path) {
    constexpr double width = 1000.0;
    constexpr double height = 650.0;
    constexpr double scale = 55.0;
    const char* colors[] = {"#2563EB", "#DC2626", "#059669", "#9333EA"};
    std::ofstream svg(path);
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1000\" height=\"650\" viewBox=\"0 0 1000 650\">\n"
        << "<rect width=\"1000\" height=\"650\" fill=\"#F8FAFC\"/>\n"
        << "<text x=\"40\" y=\"48\" font-family=\"Segoe UI\" font-size=\"28\" font-weight=\"700\" fill=\"#0F172A\">V2 Collision Avoidance Preview</text>\n"
        << "<line x1=\"40\" y1=\"325\" x2=\"960\" y2=\"325\" stroke=\"#CBD5E1\" stroke-dasharray=\"6 6\"/>\n";
    for (std::size_t i = 0; i < drones.size(); ++i) {
        svg << "<polyline fill=\"none\" stroke=\"" << colors[i % 4]
            << "\" stroke-width=\"4\" points=\"";
        for (const Vector2D& point : drones[i].trail) {
            svg << width * 0.5 + point.x * scale << ','
                << height * 0.5 - point.y * scale << ' ';
        }
        svg << "\"/>\n";
        const double tx = width * 0.5 + drones[i].target.x * scale;
        const double ty = height * 0.5 - drones[i].target.y * scale;
        svg << "<circle cx=\"" << tx << "\" cy=\"" << ty
            << "\" r=\"8\" fill=\"none\" stroke=\"" << colors[i % 4]
            << "\" stroke-width=\"3\"/>\n";
    }
    svg << "<text x=\"40\" y=\"620\" font-family=\"Segoe UI\" font-size=\"18\" fill=\"#475569\">Circle = target, line = simulated trajectory. CSV contains every 0.1 s state.</text>\n"
        << "</svg>\n";
}

} // namespace

int main() {
    std::vector<DroneSim> drones = {
        {1, {-6.0, 0.0}, {0.0, 0.0}, {6.0, 0.0}, {{-6.0, 0.0}}},
        {2, {6.0, 0.0}, {0.0, 0.0}, {-6.0, 0.0}, {{6.0, 0.0}}},
        {3, {0.0, -4.0}, {0.0, 0.0}, {0.0, 4.0}, {{0.0, -4.0}}}
    };
    const double minimum_distance = RunScenario(drones,
        "collision-avoidance-preview.csv");
    WriteSvg(drones, "collision-avoidance-preview.svg");
    double maximum_target_error = 0.0;
    for (const DroneSim& drone : drones) {
        maximum_target_error = std::max(maximum_target_error,
            Length(Subtract(drone.target, drone.position)));
    }
    std::cout << "Generated collision-avoidance-preview.svg and .csv\n"
        << "Minimum simulated pair distance: " << minimum_distance << " m\n"
        << "Maximum final target error: " << maximum_target_error << " m\n";
    if (minimum_distance < 2.0 || maximum_target_error > 0.1) {
        std::cerr << "Preview acceptance criteria failed.\n";
        return 1;
    }
    return 0;
}
