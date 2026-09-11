#pragma once
#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>
#include <mutex>
#include <map>
#include <atomic>
#include <string>

// DDS ����
#include "DroneMessage.h"
#include "../Communication/DdsService.h"

// ������ʵ�� B ģ���㷨�ӿ�[cite: 2]
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

    // �ھӱ�
    std::map<unsigned long, SharedMessage::DroneState> known_drones_;
    std::mutex neighbors_mutex_;

    // ����״̬
    double cur_x_, cur_y_;
    double cur_vx_ = 0.0, cur_vy_ = 0.0;
    double target_x_, target_y_;
    bool is_moving_ = false;

    // ҵ��ȥ����״̬��
    unsigned long last_cmd_id_ = 0;
    unsigned long seq_num_ = 0;
    SharedMessage::DroneStatus current_status_ = SharedMessage::ST_IDLE;

    std::mutex state_mutex_;
}; 
