#pragma once
#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>
#include <mutex>
#include <map>
#include <atomic>
#include <string>

// DDS 依赖
#include "DroneMessage.h"
#include "../Communication/DdsService.h"

// 引入真实的 B 模块算法接口[cite: 2]
#include "FormationAlgorithm.h"

class DroneApp {
public:
    DroneApp(unsigned long id, double init_x, double init_y);
    void Start();
    void Stop();

    void OnStateReceived(const SharedMessage::DroneState& state);
    void OnCommandReceived(const SharedMessage::DroneCmd& cmd);
    void PrintDebugStatus();

private:
    void RunLoop();
    void PublishCurrentState();

    unsigned long drone_id_;
    std::atomic<bool> running_{ false };

    // 邻居表
    std::map<unsigned long, SharedMessage::DroneState> known_drones_;
    std::mutex neighbors_mutex_;

    // 物理状态
    double cur_x_, cur_y_;
    double target_x_, target_y_;
    bool is_moving_ = false;

    // 业务去重与状态灯
    unsigned long last_cmd_id_ = 0;
    SharedMessage::DroneStatus current_status_ = SharedMessage::ST_IDLE;

    std::mutex state_mutex_;
}; 
