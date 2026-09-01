#pragma once
#include <cstdint>
#include <string>

enum class DroneCmdType : uint32_t {
    CMD_TAKEOFF = 0,
    CMD_FORMATION = 1,
    CMD_LAND = 2,
    CMD_EMERGENCY = 3
};

struct DroneCmd {
    uint32_t     cmd_id = 0;
    DroneCmdType cmd_type = DroneCmdType::CMD_TAKEOFF;
    uint32_t     formation_id = 0;
    std::string  formation_text;
    double       target_x = 0.0;
    double       target_y = 0.0;
    double       spacing = 5.0;
    uint32_t     expected_count = 8;
};