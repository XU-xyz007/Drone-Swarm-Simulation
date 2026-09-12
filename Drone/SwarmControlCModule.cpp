#include "SwarmControlCModule.h"
#include <algorithm>
#include <iostream>

SwarmControlCModule::SwarmControlCModule(uint32_t id, const V2Config& cfg)
    : my_id_(id), config_(cfg), status_(V2DroneStatus::ST_IDLE), seq_num_(0),
    pos_x_(0.0), pos_y_(0.0), vel_x_(0.0), vel_y_(0.0),
    target_x_(0.0), target_y_(0.0) {
}

void SwarmControlCModule::UpdateNeighbor(const NeighborState& neighbor) {
    if (neighbor.drone_id == my_id_) return;
    neighbors_[neighbor.drone_id] = neighbor;
}

double SwarmControlCModule::GetCurrentPublishRate() const {
    if (status_ != V2DroneStatus::ST_HOVER) {
        return config_.telemetry.active_pub_rate_hz;
    }
    for (const auto& pair : neighbors_) {
        if (pair.second.status != V2DroneStatus::ST_HOVER) {
            return config_.telemetry.active_pub_rate_hz;
        }
    }
    return config_.telemetry.static_pub_rate_hz;
}

void SwarmControlCModule::SetTarget(double tx, double ty) {
    target_x_ = tx;
    target_y_ = ty;
    status_ = V2DroneStatus::ST_IN_TRANSIT;
}

void SwarmControlCModule::SetCurrentState(double x, double y, double vx, double vy) {
    pos_x_ = x; pos_y_ = y; vel_x_ = vx; vel_y_ = vy;
}

DroneStateDDS SwarmControlCModule::RunLoop(double current_time_sec, double dt) {
    auto suspected_ids = CheckLostNeighbors(current_time_sec);
    double intent_vx = 0.0, intent_vy = 0.0;
    GenerateIntent(intent_vx, intent_vy);

    C_to_B_Request req = BuildBModuleRequest(dt, intent_vx, intent_vy);
    B_to_C_Response resp = MockBModuleCollisionAvoidance(req);
    ApplyDynamics(resp.safe_vx, resp.safe_vy, dt);

    return GenerateTelemetry(suspected_ids);
}

std::vector<uint32_t> SwarmControlCModule::CheckLostNeighbors(double current_time_sec) {
    std::vector<uint32_t> suspected;
    for (auto it = neighbors_.begin(); it != neighbors_.end(); ) {
        double expected_interval = (it->second.status == V2DroneStatus::ST_HOVER)
            ? (1.0 / config_.telemetry.static_pub_rate_hz)
            : (1.0 / config_.telemetry.active_pub_rate_hz);

        double elapsed = current_time_sec - it->second.last_receive_time_sec;
        double missed_packets = elapsed / expected_interval;

        if (missed_packets > config_.failover_quorum.max_missed_packets) {
            suspected.push_back(it->first);
            it = neighbors_.erase(it);
        }
        else {
            ++it;
        }
    }
    return suspected;
}

void SwarmControlCModule::GenerateIntent(double& ivx, double& ivy) {
    double dx = target_x_ - pos_x_;
    double dy = target_y_ - pos_y_;
    double dist = std::sqrt(dx * dx + dy * dy);

    if (dist <= config_.control_gains.deadband_radius) {
        status_ = V2DroneStatus::ST_HOVER;
        ivx = 0.0;
        ivy = 0.0;
        return;
    }

    status_ = V2DroneStatus::ST_IN_TRANSIT;
    double v_mag = config_.control_gains.kp_tracking * dist;
    if (v_mag > config_.dynamics_limits_2d.v_max) {
        v_mag = config_.dynamics_limits_2d.v_max;
    }

    ivx = (dx / dist) * v_mag;
    ivy = (dy / dist) * v_mag;
}

C_to_B_Request SwarmControlCModule::BuildBModuleRequest(double dt, double ivx, double ivy) {
    C_to_B_Request req;
    req.drone_id = my_id_;
    req.dt = dt;
    req.current_x = pos_x_; req.current_y = pos_y_;
    req.current_vx = vel_x_; req.current_vy = vel_y_;
    req.intent_vx = ivx; req.intent_vy = ivy;
    req.final_target_x = target_x_; req.final_target_y = target_y_;

    double dx = target_x_ - pos_x_;
    double dy = target_y_ - pos_y_;
    req.distance_to_target = std::sqrt(dx * dx + dy * dy);

    for (const auto& kv : neighbors_) {
        req.neighbors.push_back(kv.second);
    }
    return req;
}

B_to_C_Response SwarmControlCModule::MockBModuleCollisionAvoidance(const C_to_B_Request& req) {
    B_to_C_Response resp;
    resp.is_modified = false;
    resp.safe_vx = req.intent_vx;
    resp.safe_vy = req.intent_vy;
    return resp;
}

void SwarmControlCModule::ApplyDynamics(double safe_vx, double safe_vy, double dt) {
    if (dt <= 0.0001) return;

    double req_ax = (safe_vx - vel_x_) / dt;
    double req_ay = (safe_vy - vel_y_) / dt;
    double current_speed = std::sqrt(vel_x_ * vel_x_ + vel_y_ * vel_y_);
    double final_ax = req_ax;
    double final_ay = req_ay;

    if (current_speed > 0.01) {
        double tx = vel_x_ / current_speed;
        double ty = vel_y_ / current_speed;
        double nx = -ty;
        double ny = tx;

        double a_long = req_ax * tx + req_ay * ty;
        double a_lat = req_ax * nx + req_ay * ny;

        a_long = std::max(-config_.dynamics_limits_2d.a_max_dec,
            std::min(a_long, config_.dynamics_limits_2d.a_max_acc));
        a_lat = std::max(-config_.dynamics_limits_2d.a_max_lat,
            std::min(a_lat, config_.dynamics_limits_2d.a_max_lat));

        final_ax = a_long * tx + a_lat * nx;
        final_ay = a_long * ty + a_lat * ny;
    }
    else {
        double a_mag = std::sqrt(req_ax * req_ax + req_ay * req_ay);
        if (a_mag > config_.dynamics_limits_2d.a_max_acc) {
            final_ax = (req_ax / a_mag) * config_.dynamics_limits_2d.a_max_acc;
            final_ay = (req_ay / a_mag) * config_.dynamics_limits_2d.a_max_acc;
        }
    }

    vel_x_ += final_ax * dt;
    vel_y_ += final_ay * dt;

    double v_final_mag = std::sqrt(vel_x_ * vel_x_ + vel_y_ * vel_y_);
    if (v_final_mag > config_.dynamics_limits_2d.v_max) {
        vel_x_ = (vel_x_ / v_final_mag) * config_.dynamics_limits_2d.v_max;
        vel_y_ = (vel_y_ / v_final_mag) * config_.dynamics_limits_2d.v_max;
    }

    pos_x_ += vel_x_ * dt;
    pos_y_ += vel_y_ * dt;
}

DroneStateDDS SwarmControlCModule::GenerateTelemetry(const std::vector<uint32_t>& suspected_ids) {
    DroneStateDDS msg;
    msg.drone_id = my_id_;
    msg.status = status_;
    msg.x = pos_x_;
    msg.y = pos_y_;
    msg.vx = vel_x_;
    msg.vy = vel_y_;
    msg.current_cmd_id = 0;
    msg.suspected_lost_ids = suspected_ids;
    msg.seq_num = ++seq_num_;
    return msg;
}