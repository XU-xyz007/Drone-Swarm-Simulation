#include "DroneApp.h"
#include <vector>
#include <thread>
#include <memory>
#include <iostream>
#include <chrono>
#include <mutex>

// B 模块依赖[cite: 2]
#include "FormationAlgorithm.h"

std::vector<std::shared_ptr<DroneApp>> g_apps;
std::vector<std::thread> g_threads;
std::mutex g_apps_mutex;

int main(int argc, char* argv[]) {
    std::cout << "========== 仿真底座节点管理器 (已挂载真实 B 模块) ==========" << std::endl;

    if (!SwarmComm::InitDDS()) {
        std::cerr << "ZR-DDS 初始化失败！" << std::endl;
        return 1;
    }

    SwarmComm::SubscribeCmd([](const SharedMessage::DroneCmd& cmd) {
        std::lock_guard<std::mutex> lock(g_apps_mutex);

        if (cmd.cmd_type == SharedMessage::CMD_TAKEOFF || cmd.cmd_type == SharedMessage::CMD_FORMATION) {
            int target_count = cmd.expected_count;
            int current_count = g_apps.size();

            if (target_count > current_count) {
                int to_add = target_count - current_count;

                // 构造一次针对新生无人机的画外方阵请求[cite: 2]
                SwarmFormation::FormationRequest init_req;
                init_req.formation_id = 0; // 方阵[cite: 2]
                init_req.formation_text = "";
                init_req.count = target_count;
                init_req.center_x = 0.0;
                init_req.center_y = -50.0; // 地平线隐藏区
                init_req.spacing = cmd.spacing > 0.0 ? cmd.spacing : 2.0;

                const auto init_result = SwarmFormation::GenerateSlots(init_req);

                for (int i = 0; i < to_add; ++i) {
                    unsigned long new_id = current_count + 1 + i;
                    unsigned int my_index = new_id - 1; //[cite: 2]

                    double init_x = 0.0;
                    double init_y = -50.0;

                    if (init_result.success && my_index < init_result.slots.size()) {
                        init_x = init_result.slots[my_index].x;
                        init_y = init_result.slots[my_index].y;
                    }

                    auto app = std::make_shared<DroneApp>(new_id, init_x, init_y);
                    g_apps.push_back(app);
                    g_threads.emplace_back(&DroneApp::Start, app.get());

                    // 新无人机瞬间继承当前变阵命令并开始向上飞
                    app->OnCommandReceived(cmd);

                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
            }
        }

        // 分发给所有已存活的实例
        for (auto& app : g_apps) {
            app->OnCommandReceived(cmd);
        }
        });

    SwarmComm::SubscribeState([](const SharedMessage::DroneState& state) {
        std::lock_guard<std::mutex> lock(g_apps_mutex);
        for (auto& app : g_apps) {
            app->OnStateReceived(state);
        }
        });

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    SwarmComm::ShutdownDDS();
    return 0;
}