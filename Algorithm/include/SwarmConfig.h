#pragma once
#include <cstdint>

// 统一的集群 V2 配置文件
struct V2Config {

    // ==========================================
    // C、B 模块共享的物理参数 (B 模块只读这部分)
    // ==========================================
    struct DynamicsLimits2D {
        double v_max = 5.0;      // 适配 5 米槽位间距的最高速度
        double a_max_acc = 3.0;  // 加速度限制
        double a_max_dec = 5.0;  // 减速度限制
        double a_max_lat = 4.0;  // 横向机动限制
    } dynamics_limits_2d;

    struct AvoidanceParams {
        double drone_radius = 0.5;   // 无人机真实物理半径
        double safety_margin = 1.0;  // 基础安全余量 (配合半径推算极限护盾)
    } avoidance_params;

    // ==========================================
    // C 模块专用的业务参数 (B 模块不需要关心)
    // ==========================================
    struct Kinematics {
        double v_avg_cruise = 1.5;
        double t_min_duration = 2.0;
    } kinematics;

    struct ControlGains {
        double kp_tracking = 0.8;
        double deadband_radius = 0.1; // 针对大尺度，死区放宽到 10 厘米
    } control_gains;

    struct Telemetry {
        double active_pub_rate_hz = 10.0;
        double static_pub_rate_hz = 1.0;
    } telemetry;

    struct FailoverQuorum {
        uint32_t max_missed_packets = 3;
        uint32_t n_total_expected = 100;
    } failover_quorum;
};