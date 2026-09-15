#include "SwarmControlCModule.h"
#include "CollisionAvoidance.h"
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

    // 1. 生成初步意图速度
    GenerateIntent(intent_vx, intent_vy);

    // 2. 组装给 B 模块的请求 (完全对齐 B 模块接口)
    SwarmAvoidance::CollisionAvoidanceRequest b_req;
    b_req.drone_id = my_id_;
    b_req.dt = dt;
    b_req.current_position = { pos_x_, pos_y_ };
    b_req.current_velocity = { vel_x_, vel_y_ };
    b_req.desired_velocity = { intent_vx, intent_vy };
    b_req.target_position = { target_x_, target_y_ };

    // 动力学参数直接使用你头文件中配置的 50/15/30/15，保持高速设置
    b_req.config.max_speed = config_.dynamics_limits_2d.v_max;
    b_req.config.max_acceleration = config_.dynamics_limits_2d.a_max_acc;
    b_req.config.max_deceleration = config_.dynamics_limits_2d.a_max_dec;
    b_req.config.max_lateral_acceleration = config_.dynamics_limits_2d.a_max_lat;
    b_req.config.arrival_tolerance = config_.control_gains.deadband_radius;

    // 填充 B 模块需要的防撞判定参数 (针对 50m/s 飙车级调整)
    b_req.config.safe_distance = 5.0;          // 【护盾大小】设为 5 米，确保 UI 贴图绝对不会碰在一起
    b_req.config.warning_distance = 20.0;      // 【预警距离】速度快，必须提早在 40 米外就开始让路
    b_req.config.prediction_horizon = 2.5;     // 【预测视野】往前预测 3 秒
    b_req.config.stopped_speed_tolerance = 0.1;
    b_req.config.avoidance_gain = 2.0;         // 【避障意愿】稍微调高一点，让它躲避得更猛烈

    // 3. 转换邻居状态，建立 Arrived 映射
    for (const auto& kv : neighbors_) {
        SwarmAvoidance::NeighborState n;
        n.drone_id = kv.second.drone_id;
        n.position = { kv.second.x, kv.second.y };
        n.velocity = { kv.second.vx, kv.second.vy };

        // 核心：如果 C 模块判定对方已悬停，则让 B 模块将其视为静态障碍物
        n.motion_state = (kv.second.status == V2DroneStatus::ST_HOVER) ?
            SwarmAvoidance::NeighborMotionState::Arrived :
            SwarmAvoidance::NeighborMotionState::Moving;

        b_req.neighbors.push_back(n);
    }

    // 4. 调用真实 B 模块算法计算绝对安全速度
    SwarmAvoidance::CollisionAvoidanceResult b_result =
        SwarmAvoidance::ComputeSafeVelocity(b_req);

    static int b_log_counter = 0;
    if (++b_log_counter % 10 == 0) {
        printf("[B模块护航] 无人机 #%u | 邻居数: %zu | 避障状态: %s | 速度干预: (%.2f, %.2f) -> (%.2f, %.2f)\n",
            my_id_,
            b_req.neighbors.size(),
            SwarmAvoidance::AvoidanceStatusMessage(b_result.status), // B 模块自带的状态翻译
            intent_vx, intent_vy,
            b_result.safe_velocity.x, b_result.safe_velocity.y);
    }

    // 5. 应用最终速度到动力学约束
    ApplyDynamics(b_result.safe_velocity.x, b_result.safe_velocity.y, dt);

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

    // [核心修改] 遍历邻居时，进行状态映射
    for (const auto& kv : neighbors_) {
        BModuleNeighbor b_neighbor;
        b_neighbor.drone_id = kv.second.drone_id;
        b_neighbor.x = kv.second.x;
        b_neighbor.y = kv.second.y;
        b_neighbor.vx = kv.second.vx;
        b_neighbor.vy = kv.second.vy;

        // 状态映射：如果是悬停状态，告诉 B 模块它已到达(Arrived)，否则是移动中(Moving)
        if (kv.second.status == V2DroneStatus::ST_HOVER) {
            b_neighbor.motion_state = NeighborMotionState::Arrived;
        }
        else {
            b_neighbor.motion_state = NeighborMotionState::Moving;
        }

        req.neighbors.push_back(b_neighbor);
    }
    return req;
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