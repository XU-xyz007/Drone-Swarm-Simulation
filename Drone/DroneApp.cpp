#include "DroneApp.h"
#include <algorithm>
#include <thread>   // 引入线程支持
#include <sstream>  // 引入字符流支持，防止多线程打印错乱

// 【核心修复】初始化静态全局算力锁，保护模拟器 CPU
std::mutex DroneApp::rearrangement_mutex_;

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

    // [核心修复] 临终遗言：把自己扔到屏幕外的地底，彻底让出物理空间
    cur_x_ = 0.0;
    cur_y_ = -9999.0;

    // 发送最后一包，瞬间刷新其他所有无人机 C 模块的邻居缓存
    PublishCurrentState();
}

void DroneApp::BeginLanding() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (!running_ || landing_) return;

    // This project is two-dimensional: a landing is represented by holding
    // position, publishing ST_LANDED briefly for the UI, then ending the app.
    landing_ = true;
    landing_started_at_ = std::chrono::steady_clock::now();
    target_x_ = cur_x_;
    target_y_ = cur_y_;
    cur_vx_ = 0.0;
    cur_vy_ = 0.0;
    is_moving_ = false;
    current_status_ = SharedMessage::ST_LANDED;
    if (c_module_) c_module_->SetTarget(cur_x_, cur_y_);
}

void DroneApp::CancelLanding() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (!running_ || !landing_) return;

    landing_ = false;
    current_status_ = SharedMessage::ST_HOVER;
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
    std::vector<unsigned long> alive_ids;
    bool trigger_rearrange = false;

    // 【核心修复 1】缩小锁范围，仅用于更新物理和业务状态，坚决不在锁里算阵型
    {
        std::lock_guard<std::mutex> lock(state_mutex_);

        // 指令去重：大于本地记录才执行
        if (cmd.cmd_id <= last_cmd_id_) {
            return;
        }

        switch (cmd.cmd_type) {
        case SharedMessage::CMD_TAKEOFF:
        case SharedMessage::CMD_FORMATION: {
            // 1. 缓存主控端发来的最新队形参数
            current_formation_id_ = (cmd.cmd_type == SharedMessage::CMD_TAKEOFF && cmd.formation_id == 0) ? 0 : cmd.formation_id;
            current_formation_text_ = cmd.formation_text ? cmd.formation_text : "";
            current_center_x_ = cmd.target_x;
            current_center_y_ = cmd.target_y;
            current_spacing_ = cmd.spacing > 0.0 ? cmd.spacing : 2.0;
            active_expected_count_ = cmd.expected_count;

            // A planned scale-down removes the high IDs from this formation. Keep
            // old failover observations for active IDs only.
            confirmed_dead_.erase(
                std::remove_if(confirmed_dead_.begin(), confirmed_dead_.end(),
                    [this](unsigned long id) { return id > active_expected_count_; }),
                confirmed_dead_.end());

            // 2. 梳理存活名单
            for (unsigned long i = 1; i <= cmd.expected_count; ++i) {
                if (std::find(confirmed_dead_.begin(), confirmed_dead_.end(), i) == confirmed_dead_.end()) {
                    alive_ids.push_back(i);
                }
            }

            trigger_rearrange = true;
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
            // Avoid calling BeginLanding here: state_mutex_ is already held.
            landing_ = true;
            landing_started_at_ = std::chrono::steady_clock::now();
            target_x_ = cur_x_;
            target_y_ = cur_y_;
            is_moving_ = false;
            cur_vx_ = 0.0;
            cur_vy_ = 0.0;
            current_status_ = SharedMessage::ST_LANDED;
            if (c_module_) c_module_->SetTarget(cur_x_, cur_y_);
            break;
        }
        default:
            break;
        }

        last_cmd_id_ = cmd.cmd_id;
    } // <--- 网络接收线程在这里彻底解放！不会因为算力卡住而丢包！

    // 【核心修复 2】扔进异步线程池算，不卡 DDS 心跳
    if (trigger_rearrange) {
        std::thread([this, alive_ids]() {
            TriggerRearrangement(alive_ids);
            }).detach();
    }
}

void DroneApp::RunLoop() {
    auto last_loop_time = std::chrono::steady_clock::now();
    // C reports a newly suspected lost ID only once. Keep unsent IDs until a
    // DDS packet carrying them has been accepted by the writer.
    std::vector<unsigned long> pendingLostIds;

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
            if (landing_) {
                c_output.drone_id = drone_id_;
                c_output.x = cur_x_;
                c_output.y = cur_y_;
                c_output.vx = 0.0;
                c_output.vy = 0.0;
                c_output.status = V2DroneStatus::ST_LANDED;
            }
            else if (c_module_) {
                // 将最新物理状态灌入 C 模块
                c_module_->SetCurrentState(cur_x_, cur_y_, cur_vx_, cur_vy_);

                // 执行核心流水线 
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

        // 分离读锁，安全读取将要发包的坐标
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            state.x = cur_x_;
            state.y = cur_y_;
            state.vx = cur_vx_;
            state.vy = cur_vy_;
            state.current_cmd_id = last_cmd_id_;
        }
        state.seq_num = c_output.seq_num;

        // DroneMessage.idl bounds suspected_lost_ids to 10 entries. 
        constexpr size_t kMaxSuspectedLostIdsPerPacket = 10;
        for (unsigned long id : c_output.suspected_lost_ids) {
            if (std::find(pendingLostIds.begin(), pendingLostIds.end(), id) ==
                pendingLostIds.end()) {
                pendingLostIds.push_back(id);
            }
        }

        const size_t batchSize = (std::min)(kMaxSuspectedLostIdsPerPacket,
            pendingLostIds.size());
        bool lostBatchAttached = false;
        if (batchSize != 0 && state.suspected_lost_ids.ensure_length(
            static_cast<unsigned long>(batchSize),
            static_cast<unsigned long>(batchSize))) {
            for (size_t i = 0; i < batchSize; ++i) {
                state.suspected_lost_ids[i] = pendingLostIds[i];
            }
            lostBatchAttached = true;
        }

        bool ok = SwarmComm::PublishState(state);
        if (ok && lostBatchAttached) {
            pendingLostIds.erase(pendingLostIds.begin(),
                pendingLostIds.begin() + batchSize);
        }
        if (!ok) {
            std::cerr << "[网络警告] 无人机 #" << state.drone_id << " 状态包发送失败！" << std::endl;
        }

        // 释放 DDS 内部深拷贝内存
        SharedMessage::DroneStateFinalize(&state);

        // Keep ST_LANDED visible long enough for the controller UI to play fade-out
        if (landing_ && std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - landing_started_at_).count() >= 1200) {
            running_ = false;
        }

        // --- 3. 严格补偿休眠 ---
        double target_rate = c_module_ ? c_module_->GetCurrentPublishRate() : 10.0;
        int sleep_ms = static_cast<int>(1000.0 / target_rate);

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - current_time).count();

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

void DroneApp::TriggerRearrangement(const std::vector<unsigned long>& alive_ids) {
    if (alive_ids.empty()) return;

    // 1. 将所有存活的 ID 排序
    std::vector<unsigned long> sorted_ids = alive_ids;
    std::sort(sorted_ids.begin(), sorted_ids.end());

    // 2. 找到我自己在存活列表中的绝对名次
    unsigned int new_my_index = 0;
    bool i_am_alive = false;
    for (size_t i = 0; i < sorted_ids.size(); ++i) {
        if (sorted_ids[i] == drone_id_) {
            new_my_index = i;
            i_am_alive = true;
            break;
        }
    }

    if (!i_am_alive) return;

    // 【核心修复 3】从状态机中给参数“拍快照”，防止多线程竞争导致参数错乱
    SwarmFormation::FormationRequest req;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        req.formation_id = current_formation_id_;
        req.formation_text = current_formation_text_;
        req.count = sorted_ids.size();
        req.center_x = current_center_x_;
        req.center_y = current_center_y_;
        req.spacing = current_spacing_;
    }

    // 【核心修复 4】排队执行 O(N^3) 的耗时算法，保护电脑 CPU 不被海量线程撑爆
    SwarmFormation::FormationResult result;
    {
        std::lock_guard<std::mutex> calc_lock(rearrangement_mutex_);
        result = SwarmFormation::GenerateSlots(req);
    }

    // 4. 计算完成，重新拿锁，将槽位赋予物理控制器
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (result.success && new_my_index < result.slots.size()) {
            target_x_ = result.slots[new_my_index].x;
            target_y_ = result.slots[new_my_index].y;
            is_moving_ = true;
            if (c_module_) c_module_->SetTarget(target_x_, target_y_);

            std::ostringstream oss;
            oss << "[自治重排] 无人机 #" << drone_id_
                << " 获取新索引 " << new_my_index
                << "，前往新槽位 (" << target_x_ << ", " << target_y_ << ")\n";
            std::cout << oss.str();
        }
        else {
            std::ostringstream oss;
            oss << "[Drone " << drone_id_ << " 算法拒载] "
                << SwarmFormation::FormationErrorMessage(result.error) << "\n";
            std::cerr << oss.str();
        }
    }
}