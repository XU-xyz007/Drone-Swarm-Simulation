#pragma once
#include <cstdint>
#include <string>
#include <vector>

enum class MovePattern : uint32_t {
    RANDOM_WALK = 0,
    CIRCULAR = 1,
    WAVE = 2,
    ZIGZAG = 3,
    FORMATION_CYCLE = 4
};

struct StressPreset {
    std::string name;
    std::string description;
    uint32_t droneCount;
    int updateHz;
    double speed;
    MovePattern pattern;
    bool autoFormation;
    int formationIntervalTicks;
    uint32_t expectedMinFps;

    static std::vector<StressPreset> all();
};

inline std::vector<StressPreset> StressPreset::all() {
    return {
        {
            "P1-基础轻载",
            "10架无人机，10Hz更新，随机游走，速度3.0 — 基线对比",
            10, 10, 3.0, MovePattern::RANDOM_WALK, false, 0, 55
        },
        {
            "P2-中等负载",
            "50架无人机，10Hz更新，随机游走，速度3.0 — 中等数量",
            50, 10, 3.0, MovePattern::RANDOM_WALK, false, 0, 55
        },
        {
            "P3-高负载",
            "100架无人机，10Hz更新，随机游走，速度3.0 — 大量无人机",
            100, 10, 3.0, MovePattern::RANDOM_WALK, false, 0, 50
        },
        {
            "P4-极限数量",
            "200架无人机，10Hz更新，随机游走，速度3.0 — 极限数量",
            200, 10, 3.0, MovePattern::RANDOM_WALK, false, 0, 40
        },
        {
            "P5-高频更新",
            "50架无人机，50Hz更新，随机游走，速度3.0 — 高频消息压力",
            50, 50, 3.0, MovePattern::RANDOM_WALK, false, 0, 50
        },
        {
            "P6-高频+大量",
            "100架无人机，50Hz更新，随机游走，速度5.0 — 高频+大量",
            100, 50, 5.0, MovePattern::RANDOM_WALK, false, 0, 40
        },
        {
            "P7-极限综合",
            "200架无人机，50Hz更新，随机游走，速度5.0 — 极限综合压力",
            200, 50, 5.0, MovePattern::RANDOM_WALK, false, 0, 30
        },
        {
            "P8-高速移动",
            "50架无人机，10Hz更新，高速移动(10.0) — 渲染拖影压力",
            50, 10, 10.0, MovePattern::RANDOM_WALK, false, 0, 55
        },
        {
            "P9-圆形轨迹",
            "80架无人机，10Hz更新，圆形轨道运动 — 均匀分布渲染",
            80, 10, 3.0, MovePattern::CIRCULAR, false, 0, 55
        },
        {
            "P10-波浪运动",
            "60架无人机，10Hz更新，正弦波运动 — 规律性移动",
            60, 10, 3.0, MovePattern::WAVE, false, 0, 55
        },
        {
            "P11-折线运动",
            "70架无人机，10Hz更新，Z形折线运动 — 方向突变测试",
            70, 10, 3.0, MovePattern::ZIGZAG, false, 0, 55
        },
        {
            "P12-队形轮换",
            "50架无人机，10Hz更新，每3秒自动切换队形 — 队形判定压力",
            50, 10, 3.0, MovePattern::FORMATION_CYCLE, true, 30, 50
        },
        {
            "P13-队形密集",
            "120架无人机，10Hz更新，每2秒切换队形，间距15 — 密集队形",
            120, 10, 3.0, MovePattern::FORMATION_CYCLE, true, 20, 45
        },
        {
            "P14-混合状态",
            "50架无人机，10Hz更新，含急停/降落/故障 — 多状态渲染",
            50, 10, 3.0, MovePattern::RANDOM_WALK, false, 0, 55
        },
        {
            "P15-极限队形",
            "200架无人机，20Hz更新，每1.5秒换队形，高速 — 极限队形压力",
            200, 20, 6.0, MovePattern::FORMATION_CYCLE, true, 15, 30
        },
    };
}