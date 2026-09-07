#include "DroneApp.h"
#include <vector>
#include <thread>
#include <memory>
#include <iostream>
#include <chrono>
#include <mutex>
#include <atomic>

// B 模块依赖
#include "FormationAlgorithm.h"

// 全局节点管理池
std::vector<std::shared_ptr<DroneApp>> g_apps;
std::vector<std::thread> g_threads;
std::mutex g_apps_mutex;

int main(int argc, char* argv[]) {
    std::cout << "========== 无人机底座节点管理器 (静默模式) ==========" << std::endl;
    std::cout << "-> 正在初始化 DDS 通信中间件..." << std::endl;

    if (!SwarmComm::InitDDS()) {
        std::cerr << "ZR-DDS 初始化失败！请确保 ZRDDS_QOS_PROFILES.xml 在同一目录下。" << std::endl;
        return 1;
    }

    std::cout << "-> DDS 就绪。挂载后台，等待主控端唤醒..." << std::endl;

    // ================= 1. 订阅主控端指令 =================
    SwarmComm::SubscribeCmd([](const SharedMessage::DroneCmd& cmd) {
        // [新增] 首次接触日志：仅在收到第一条指令时打印一次，证明链路已打通
        static std::atomic<bool> first_contact{ false };
        if (!first_contact.exchange(true)) {
            std::cout << "\n[系统日志] 通信链路打通！成功接收到主控端首次指令："
                << "cmd_id=" << cmd.cmd_id
                << " | 目标数量=" << cmd.expected_count << " 架" << std::endl;
        }

        std::lock_guard<std::mutex> lock(g_apps_mutex);

        // 动态扩容与画外降生逻辑
        if (cmd.cmd_type == SharedMessage::CMD_TAKEOFF || cmd.cmd_type == SharedMessage::CMD_FORMATION) {
            int target_count = cmd.expected_count;
            int current_count = g_apps.size();

            if (target_count > current_count) {
                int to_add = target_count - current_count;

                // 构造针对新生无人机的“地下待机”方阵请求
                SwarmFormation::FormationRequest init_req;
                init_req.formation_id = 0; // 方阵
                init_req.formation_text = "";
                init_req.count = target_count;
                init_req.center_x = 0.0;
                init_req.center_y = -50.0; // 地平线隐藏区，适配 A 端 Y>0 的渲染过滤
                init_req.spacing = cmd.spacing > 0.0 ? cmd.spacing : 2.0;

                const auto init_result = SwarmFormation::GenerateSlots(init_req);

                for (int i = 0; i < to_add; ++i) {
                    unsigned long new_id = current_count + 1 + i;
                    unsigned int my_index = new_id - 1;

                    double init_x = 0.0;
                    double init_y = -50.0;

                    // 读取 B 模块算出的槽位进行降生
                    if (init_result.success && my_index < init_result.slots.size()) {
                        init_x = init_result.slots[my_index].x;
                        init_y = init_result.slots[my_index].y;
                    }

                    auto app = std::make_shared<DroneApp>(new_id, init_x, init_y);
                    g_apps.push_back(app);
                    g_threads.emplace_back(&DroneApp::Start, app.get());

                    // 让新生无人机立刻执行当前变阵命令并升空
                    app->OnCommandReceived(cmd);

                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
            }
        }

        // 无论是否扩容，都将新指令分发给所有存活的实体实例
        for (auto& app : g_apps) {
            app->OnCommandReceived(cmd);
        }
        });

    // ================= 2. 订阅邻居状态更新 =================
    SwarmComm::SubscribeState([](const SharedMessage::DroneState& state) {
        std::lock_guard<std::mutex> lock(g_apps_mutex);
        for (auto& app : g_apps) {
            app->OnStateReceived(state);
        }
        });

    // ================= 3. 守护进程主循环 (带监控仪表盘) =================
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::lock_guard<std::mutex> lock(g_apps_mutex);
        if (!g_apps.empty()) {
            std::cout << "\n=== [后台监控] 当前集群规模: " << g_apps.size() << " 架 ===" << std::endl;
            // 为了不刷屏，只打印前 3 架无人机的动态
            int print_count = g_apps.size() > 3 ? 3 : g_apps.size();
            for (int i = 0; i < print_count; ++i) {
                g_apps[i]->PrintDebugStatus();
            }
        }
    }

    SwarmComm::ShutdownDDS();
    return 0;
}