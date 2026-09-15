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
#include "SwarmControlCModule.h"


class DroneApp {
public:
    DroneApp(unsigned long id, double init_x, double init_y);
    ~DroneApp();
    void Start();
    void Stop();

    void OnStateReceived(const SharedMessage::DroneState& state);
    void OnCommandReceived(const SharedMessage::DroneCmd& cmd);
    void PrintDebugStatus();
    // 1. 新增一把算力锁，防止疯狂掉线时引发“线程爆炸”把 CPU 算崩
    std::mutex rearrangement_mutex_;

    // 2. 修改重排函数的声明（增加 req 参数用于快照传递）
    void TriggerRearrangement(const std::vector<unsigned long>& alive_ids, SwarmFormation::FormationRequest req);

    bool IsRunning() const { return running_; }

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
    double cur_vx_ = 0.0, cur_vy_ = 0.0;
    double target_x_, target_y_;
    bool is_moving_ = false;

    // 业务去重与状态灯
    unsigned long last_cmd_id_ = 0;
    unsigned long seq_num_ = 0;
    SharedMessage::DroneStatus current_status_ = SharedMessage::ST_IDLE;

    V2Config swarm_config_;                   // 集群配置
    SwarmControlCModule* c_module_ = nullptr; // C模块指针

    // 记录系统启动时间，用于计算 current_time_sec
    std::chrono::steady_clock::time_point app_start_time_;

    std::mutex state_mutex_;

    // 在 private 区域添加：
    int current_formation_id_ = 0;
    double current_center_x_ = 0.0;
    double current_center_y_ = 0.0;
    double current_spacing_ = 2.0;
    std::string current_formation_text_ = "";

    std::vector<unsigned long> confirmed_dead_; // 记录已确认阵亡的名单
}; 
