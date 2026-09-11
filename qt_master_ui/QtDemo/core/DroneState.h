#pragma once
#include <cstdint>

enum class DroneStatus : uint32_t {
    ST_IDLE = 0,
    ST_TAKEOFF = 1,
    ST_CRUISE = 2,
    ST_LANDING = 3,
    ST_LANDED = 4,
    ST_FAULT = 5,
    ST_LOST = 6,
    ST_HOVER = 7,
    ST_IN_TRANSIT = 8,
    ST_FALLBACK_CATCHING = 9
};

struct DroneState {
    uint32_t    drone_id = 0;
    DroneStatus status = DroneStatus::ST_IDLE;
    double      x = 0.0;
    double      y = 0.0;
    double      vx = 0.0;
    double      vy = 0.0;
    uint32_t    current_cmd_id = 0;
    uint32_t    seq_num = 0;
    bool        transitioning = false;
};
