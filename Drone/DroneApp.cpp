#include "DroneApp.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <thread> // 引入线程支持

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
    SwarmFormation::FormationRequest snapshot_req; // 【核心修复】保存参数快照

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

            // 制作快照，供异步线程安全使用，防止数据竞争导致文字丢失
            snapshot_req.formation_id = current_formation_id_;
            snapshot_req.formation_text = current_formation_text_;
            snapshot_req.center_x = current_center_x_;
            snapshot_req.center_y = current_center_y_;
            snapshot_req.spacing = current_spacing_;

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
    } // <--- 锁在这里被释放

    // 【核心修复】将重排扔进后台，让网络接收线程瞬间返回，绝不卡死心跳！
    if (trigger_rearrange) {
        std::thread([this, alive_ids, snapshot_req]() {
            TriggerRearrangement(alive_ids, snapshot_req);
            }).detach();
    }
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

        bool new_death_detected = false;

        // --- 1. 执行 C 模块 10Hz 闭环 ---
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (c_module_) {
                // 将最新物理状态灌入 C 模块
                c_module_->SetCurrentState(cur_x_, cur_y_, cur_vx_, cur_vy_);

                // 执行核心流水线 
                c_output = c_module_->RunLoop(current_time_sec, dt);

                // =============================================================
                // 掉线检测与自治重排触发器
                for (unsigned long lost_id : c_output.suspected_lost_ids) {
                    if (std::find(confirmed_dead_.begin(), confirmed_dead_.end(), lost_id) == confirmed_dead_.end()) {
                        confirmed_dead_.push_back(lost_id);
                        new_death_detected = true;
                    }
                }

                // 更新本机真实物理量
                cur_x_ = c_output.x;
                cur_y_ = c_output.y;
                cur_vx_ = c_output.vx;
                cur_vy_ = c_output.vy;
                current_status_ = static_cast<SharedMessage::DroneStatus>(c_output.status);
            }
        } // <--- 提前释放 state_mutex_ 锁

        // 2. 只要名单有更新，立刻集结剩下的人重排
        if (new_death_detected) {
            std::vector<unsigned long> alive_ids;
            SwarmFormation::FormationRequest snapshot_req;
            alive_ids.push_back(drone_id_);

            // 安全获取最新邻居存活名单
            {
                std::lock_guard<std::mutex> n_lock(neighbors_mutex_);
                for (const auto& pair : known_drones_) {
                    unsigned long neighbor_id = pair.first;
                    if (std::find(confirmed_dead_.begin(), confirmed_dead_.end(), neighbor_id) == confirmed_dead_.end()) {
                        alive_ids.push_back(neighbor_id);
                    }
                }
            }

            // 安全获取当前的阵型参数快照
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                snapshot_req.formation_id = current_formation_id_;
                snapshot_req.formation_text = current_formation_text_;
                snapshot_req.center_x = current_center_x_;
                snapshot_req.center_y = current_center_y_;
                snapshot_req.spacing = current_spacing_;
            }

            // 【核心修复】开启异步线程执行自治重排，绝对不阻塞 RunLoop 发包！
            std::thread([this, alive_ids, snapshot_req]() {
                TriggerRearrangement(alive_ids, snapshot_req);
                }).detach();
        }

        // --- 2. 组装并发布 IDL 结构包 ---
        SharedMessage::DroneState state;
        SharedMessage::DroneStateInitialize(&state);

        state.drone_id = c_output.drone_id;
        state.status = static_cast<SharedMessage::DroneStatus>(c_output.status);

        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            state.x = cur_x_;
            state.y = cur_y_;
            state.vx = cur_vx_;
            state.vy = cur_vy_;
            state.current_cmd_id = last_cmd_id_;
        }
        state.seq_num = c_output.seq_num;

        if (!c_output.suspected_lost_ids.empty()) {
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

        SharedMessage::DroneStateFinalize(&state);

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

// 【核心修复】防止疯狂掉线时引发“线程爆炸”，保证同一时刻只有一次重排在算
void DroneApp::TriggerRearrangement(const std::vector<unsigned long>& alive_ids, SwarmFormation::FormationRequest req) {
    std::lock_guard<std::mutex> calc_lock(rearrangement_mutex_);

    if (alive_ids.empty()) return;

    std::vector<unsigned long> sorted_ids = alive_ids;
    std::sort(sorted_ids.begin(), sorted_ids.end());

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

    // 解决控制台穿插错乱问题
    std::ostringstream oss;
    oss << "[DEBUG 传参排查] drone_id=" << drone_id_
        << " | formation_id=" << req.formation_id
        << " | text=" << (req.formation_text.empty() ? "NULL/EMPTY" : req.formation_text)
        << " | count=" << sorted_ids.size()
        << " | center=(" << req.center_x << ", " << req.center_y << ")\n";
    std::cout << oss.str();

    // 直接使用传进来的快照计算
    req.count = sorted_ids.size();
    auto result = SwarmFormation::GenerateSlots(req);

    // 计算完毕后，重新锁定 state_mutex_ 以应用物理参数
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (result.success && new_my_index < result.slots.size()) {
            target_x_ = result.slots[new_my_index].x;
            target_y_ = result.slots[new_my_index].y;
            is_moving_ = true;
            if (c_module_) c_module_->SetTarget(target_x_, target_y_);

            std::ostringstream oss_succ;
            oss_succ << "[自治重排] 无人机 #" << drone_id_
                << " 获取新索引 " << new_my_index
                << "，前往新槽位 (" << target_x_ << ", " << target_y_ << ")\n";
            std::cout << oss_succ.str();
        }
        else {
            std::ostringstream oss_err;
            oss_err << "[Drone " << drone_id_ << " 算法拒载] "
                << SwarmFormation::FormationErrorMessage(result.error)
                << " -> 触发安全回退：保持当前目标点继续飞行/悬停。\n";
            std::cerr << oss_err.str();

            is_moving_ = true;
            if (c_module_) {
                c_module_->SetTarget(target_x_, target_y_);
            }
        }
    }
}
