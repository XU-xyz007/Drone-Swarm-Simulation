#include "DroneApp.h"

DroneApp::DroneApp(unsigned long id, double init_x, double init_y)
    : drone_id_(id), cur_x_(init_x), cur_y_(init_y),
    target_x_(init_x), target_y_(init_y) {
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
                current_status_ = SharedMessage::ST_CRUISE;
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
        break;
    }
    case SharedMessage::CMD_LAND: {
        target_x_ = cur_x_;
        target_y_ = cur_y_;
        is_moving_ = false;
        current_status_ = SharedMessage::ST_LANDED;
        break;
    }
    default:
        break;
    }

    last_cmd_id_ = cmd.cmd_id;
}

void DroneApp::RunLoop() {
    const double dt = 0.1;
    const double arrive_threshold = 0.05;
    const double max_speed = 2.0;
    int hover_tick_counter = 0;

    while (running_) {
        auto start_time = std::chrono::steady_clock::now();
        bool current_is_moving = false;

        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            current_is_moving = is_moving_;

            if (is_moving_) {
                double dx = target_x_ - cur_x_;
                double dy = target_y_ - cur_y_;
                double dist = std::hypot(dx, dy);
                double step = max_speed * dt;

                if (dist <= step || dist <= arrive_threshold) {
                    cur_x_ = target_x_;
                    cur_y_ = target_y_;
                    is_moving_ = false;
                    current_is_moving = false;
                    if (current_status_ == SharedMessage::ST_CRUISE) {
                        current_status_ = SharedMessage::ST_IDLE;
                    }
                }
                else {
                    cur_x_ += (dx / dist) * step;
                    cur_y_ += (dy / dist) * step;
                }
            }
        }

        // 动静分离的自适应发信
        if (current_is_moving) {
            PublishCurrentState();
            hover_tick_counter = 0;
        }
        else {
            if (hover_tick_counter == 0 || hover_tick_counter >= 10) {
                PublishCurrentState();
                hover_tick_counter = 1;
            }
            else {
                hover_tick_counter++;
            }
        }

        // 100ms 严格补偿休眠
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
        if (elapsed < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100 - elapsed));
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
        state.status = is_moving_ ? SharedMessage::ST_CRUISE : current_status_;
    }

    SwarmComm::PublishState(state);
}