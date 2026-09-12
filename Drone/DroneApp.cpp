#include "DroneApp.h"

DroneApp::DroneApp(unsigned long id, double init_x, double init_y)
    : drone_id_(id), cur_x_(init_x), cur_y_(init_y),
    target_x_(init_x), target_y_(init_y) {

    app_start_time_ = std::chrono::steady_clock::now();

    // 实例化 C 模块
    c_module_ = new SwarmControlCModule(id, swarm_config_);
    c_module_->SetCurrentState(init_x, init_y, 0.0, 0.0);
    c_module_->SetTarget(init_x, init_y);
}

DroneApp::~DroneApp() {
    if (c_module_) delete c_module_;
}

void DroneApp::Start() {
    running_ = true;
    RunLoop();
}

void DroneApp::Stop() {
    running_ = false;
}

void DroneApp::OnStateReceived(const SharedMessage::DroneState& state) {
    if (state.drone_id == drone_id_) return;

    std::lock_guard<std::mutex> lock(neighbors_mutex_);
    known_drones_[state.drone_id] = state;

    NeighborState ns;
    ns.drone_id = state.drone_id;
    ns.x = state.x;
    ns.y = state.y;
    ns.vx = state.vx;
    ns.vy = state.vy;
    ns.status = static_cast<V2DroneStatus>(state.status);

    // 获取当前时间秒数，喂给容灾仲裁器
    auto now = std::chrono::steady_clock::now();
    double current_time_sec = std::chrono::duration<double>(now - app_start_time_).count();
    ns.last_receive_time_sec = current_time_sec;

    // 3. 喂给 C 模块
    if (c_module_) {
        c_module_->UpdateNeighbor(ns);
    }
}

void DroneApp::OnCommandReceived(const SharedMessage::DroneCmd& cmd) {
    std::lock_guard<std::mutex> lock(state_mutex_);

    // 指令去重：大于本地记录才执行
    if (cmd.cmd_id <= last_cmd_id_) {
        return;
    }

    // 将在线的 drone_id (1-N) 升序映射为从 0 开始的连续 index[cite: 2]
    unsigned int my_index = drone_id_ - 1;

    switch (cmd.cmd_type) {
    case SharedMessage::CMD_TAKEOFF:
    case SharedMessage::CMD_FORMATION: {
        // 构造真实的 B 模块算法请求[cite: 2]
        SwarmFormation::FormationRequest request;
        request.formation_id = (cmd.cmd_type == SharedMessage::CMD_TAKEOFF && cmd.formation_id == 0) ? 0 : cmd.formation_id;
        request.formation_text = cmd.formation_text ? cmd.formation_text : "";
        request.count = cmd.expected_count;
        request.center_x = cmd.target_x;
        request.center_y = cmd.target_y;
        request.spacing = cmd.spacing > 0.0 ? cmd.spacing : 2.0;

        // 调用 B 模块生成所有槽位[cite: 2]
        const auto result = SwarmFormation::GenerateSlots(request);

        if (result.success) {
            if (my_index < result.slots.size()) {
                target_x_ = result.slots[my_index].x;
                target_y_ = result.slots[my_index].y;
                is_moving_ = true;
                if (c_module_) {
                    c_module_->SetTarget(target_x_, target_y_);
                }
            }
        }
        else {
            // 如果算法报错，打印 B 模块提供的规范错误信息[cite: 2]
            std::cerr << "[Drone " << drone_id_ << " 算法拒载] "
                << SwarmFormation::FormationErrorMessage(result.error) << std::endl;
        }
        break;
    }
    case SharedMessage::CMD_EMERGENCY: {
        target_x_ = cur_x_;
        target_y_ = cur_y_;
        is_moving_ = false;
        
        if (c_module_) {
            c_module_->SetTarget(cur_x_, cur_y_);
        }
        break;
    }
    case SharedMessage::CMD_LAND: {
        target_x_ = cur_x_;
        target_y_ = cur_y_;
        is_moving_ = false;
        cur_vx_ = 0.0;
        cur_vy_ = 0.0;
        current_status_ = SharedMessage::ST_LANDED;
        break;
    }
    default:
        break;
    }

    last_cmd_id_ = cmd.cmd_id;
}

void DroneApp::RunLoop() {
    auto last_loop_time = std::chrono::steady_clock::now();

    while (running_) {
        auto current_time = std::chrono::steady_clock::now();

        // 计算物理积分步长 dt 和 运行绝对时间（秒）
        double dt = std::chrono::duration<double>(current_time - last_loop_time).count();
        double current_time_sec = std::chrono::duration<double>(current_time - app_start_time_).count();
        last_loop_time = current_time;

        DroneStateDDS c_output;
        c_output.drone_id = drone_id_;
        c_output.status = static_cast<V2DroneStatus>(current_status_);

        // --- 1. 执行 C 模块 10Hz 闭环 ---
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (c_module_) {
                // 将最新物理状态灌入 C 模块
                c_module_->SetCurrentState(cur_x_, cur_y_, cur_vx_, cur_vy_);

                // 执行核心流水线 (意图生成 -> Mock避障 -> 动力学平滑 -> 遥测打包)
                c_output = c_module_->RunLoop(current_time_sec, dt);

                // 根据 C 模块的动力学计算结果更新本机真实物理量
                cur_x_ = c_output.x;
                cur_y_ = c_output.y;
                cur_vx_ = c_output.vx;
                cur_vy_ = c_output.vy;
                current_status_ = static_cast<SharedMessage::DroneStatus>(c_output.status);
            }
        }

        // --- 2. 组装并发布 IDL 结构包 ---
        SharedMessage::DroneState state;
        SharedMessage::DroneStateInitialize(&state);

        state.drone_id = c_output.drone_id;
        state.status = static_cast<SharedMessage::DroneStatus>(c_output.status);
        state.x = cur_x_;
        state.y = cur_y_;
        state.vx = cur_vx_;
        state.vy = cur_vy_;
        state.current_cmd_id = last_cmd_id_;
        state.seq_num = c_output.seq_num;

        // 填充底层 DDS Sequence 数组，广播容灾怀疑失联 ID 列表
        if (!c_output.suspected_lost_ids.empty()) {
            // ZRDDS/DDS 序列标准分配长度
            state.suspected_lost_ids.ensure_length(
                static_cast<unsigned long>(c_output.suspected_lost_ids.size()),
                static_cast<unsigned long>(c_output.suspected_lost_ids.size())
            );
            for (size_t i = 0; i < c_output.suspected_lost_ids.size(); ++i) {
                state.suspected_lost_ids[i] = c_output.suspected_lost_ids[i];
            }
        }

        bool ok = SwarmComm::PublishState(state);
        if (!ok) {
            std::cerr << "[网络警告] 无人机 #" << state.drone_id << " 状态包发送失败！" << std::endl;
        }

        // 释放 DDS 内部深拷贝内存
        SharedMessage::DroneStateFinalize(&state);

        // --- 3. 严格补偿休眠 (实现基于全员静止原则的频率动态降级) ---
        // 若 C 模块判定全员 HOVER，将返回 1Hz；若存在动态机动，返回 10Hz
        double target_rate = c_module_ ? c_module_->GetCurrentPublishRate() : 10.0;
        int sleep_ms = static_cast<int>(1000.0 / target_rate);

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - current_time).count();

        // 扣除本轮算法执行与发包耗时，进行精确睡眠
        if (elapsed < sleep_ms) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms - elapsed));
        }
    }
}

void DroneApp::PublishCurrentState() {
    SharedMessage::DroneState state;
    SharedMessage::DroneStateInitialize(&state);

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        state.drone_id = drone_id_;
        state.current_cmd_id = last_cmd_id_;
        state.x = cur_x_;
        state.y = cur_y_;
        state.vx = cur_vx_;
        state.vy = cur_vy_;
        state.seq_num = ++seq_num_;
        state.status = is_moving_ ? SharedMessage::ST_IN_TRANSIT : current_status_;
    }

    bool ok = SwarmComm::PublishState(state);
    if (!ok) {
        std::cerr << "[网络警告] 无人机 #" << state.drone_id << " 状态包发送失败！" << std::endl;
    }
    SharedMessage::DroneStateFinalize(&state);
}

void DroneApp::PrintDebugStatus() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    std::cout << "  -> 无人机 #" << drone_id_
        << " | 当前坐标: (" << cur_x_ << ", " << cur_y_ << ")"
        << " | 目标槽位: (" << target_x_ << ", " << target_y_ << ")"
        << " | 状态码: " << current_status_ << std::endl;
}
