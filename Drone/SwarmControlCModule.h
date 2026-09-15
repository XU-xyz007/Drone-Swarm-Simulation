#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <cmath>
#include "SwarmConfig.h"

// ============================================================================
// 数据结构契约定义 (已改名为 V2DroneStatus，彻底杜绝底层冲突)
// ============================================================================

enum class V2DroneStatus : uint32_t {
    ST_IDLE = 0, ST_TAKEOFF, ST_CRUISE, ST_LANDING, ST_LANDED, ST_FAULT, ST_LOST,
    ST_HOVER,
    ST_IN_TRANSIT,
    ST_FALLBACK_CATCHING
};

enum class NeighborMotionState {
    Unknown = 0,
    Moving,
    Arrived
};

struct BModuleNeighbor {
    uint32_t drone_id;
    double x;
    double y;
    double vx;
    double vy;
    NeighborMotionState motion_state; // 满足 B 模块的第五条需求
};

struct DroneStateDDS {
    uint32_t drone_id;
    V2DroneStatus status;
    double x;
    double y;
    uint32_t current_cmd_id;
    double vx;
    double vy;
    std::vector<uint32_t> suspected_lost_ids;
    uint32_t seq_num;
};

struct NeighborState {
    uint32_t drone_id;
    double x;
    double y;
    double vx;
    double vy;
    V2DroneStatus status;
    double last_receive_time_sec;
};

struct C_to_B_Request {
    uint32_t drone_id;
    double dt;
    double current_x;
    double current_y;
    double current_vx;
    double current_vy;
    double intent_vx;
    double intent_vy;
    double final_target_x;
    double final_target_y;
    double distance_to_target;
    std::vector<BModuleNeighbor> neighbors;
};

struct B_to_C_Response {
    bool is_modified;
    double safe_vx;
    double safe_vy;
};


class SwarmControlCModule {
public:
    SwarmControlCModule(uint32_t id, const V2Config& cfg);
    void UpdateNeighbor(const NeighborState& neighbor);
    double GetCurrentPublishRate() const;
    DroneStateDDS RunLoop(double current_time_sec, double dt);
    void SetTarget(double tx, double ty);
    void SetCurrentState(double x, double y, double vx, double vy);

private:
    std::vector<uint32_t> CheckLostNeighbors(double current_time_sec);
    void GenerateIntent(double& ivx, double& ivy);
    C_to_B_Request BuildBModuleRequest(double dt, double ivx, double ivy);
    void ApplyDynamics(double safe_vx, double safe_vy, double dt);
    DroneStateDDS GenerateTelemetry(const std::vector<uint32_t>& suspected_ids);

private:
    uint32_t my_id_;
    V2Config config_;
    V2DroneStatus status_;
    uint32_t seq_num_;

    double pos_x_, pos_y_;
    double vel_x_, vel_y_;
    double target_x_, target_y_;
    std::unordered_map<uint32_t, NeighborState> neighbors_;
};