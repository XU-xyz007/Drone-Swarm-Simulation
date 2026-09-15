#include <conio.h>    // 提供 _kbhit() 和 _getch()
#include <random>     // 提供 std::mt19937
#include <algorithm>  // 提供 std::shuffle
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

// [新增] 模拟无人机随机坠毁/失联
void SimulateCrash(double ratio) {
    std::lock_guard<std::mutex> lock(g_apps_mutex);
    std::vector<std::shared_ptr<DroneApp>> alive_drones;

    // 1. 筛选当前仍在运行的无人机
    for (auto& app : g_apps) {
        // 注意：这需要你在 DroneApp.h 中暴露出 public: bool IsRunning() const { return running_; }
        if (app->IsRunning()) {
            alive_drones.push_back(app);
        }
    }

    int kill_count = static_cast<int>(alive_drones.size() * ratio);
    if (kill_count == 0) return;

    // 2. 随机打乱存活列表
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(alive_drones.begin(), alive_drones.end(), gen);

    // 3. 强制关闭被选中的无人机发包循环
    for (int i = 0; i < kill_count; ++i) {
        alive_drones[i]->Stop();
    }

    std::cout << "\n======================================================\n";
    std::cout << "[突发演习] 强行断联 " << (ratio * 100) << "% 的无人机，共 " << kill_count << " 架已失联！\n";
    std::cout << "======================================================\n";
}

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

    // ================= 3. 守护进程主循环 (带键盘监听与仪表盘) =================
    auto last_print_time = std::chrono::steady_clock::now();

    while (true) {
        // 1. 非阻塞按键监听 (不卡顿)
        if (_kbhit()) {
            char ch = _getch();
            switch (ch) {
            case '1': SimulateCrash(0.10); break; // 杀 10%
            case '2': SimulateCrash(0.15); break; // 杀 15%
            case '3': SimulateCrash(0.20); break; // 杀 20%
            case '4': SimulateCrash(0.30); break; // 杀 30%
            default: break;
            }
        }

        // 2. 维持原有的仪表盘打印逻辑 (每 2 秒打印一次，防止刷屏)
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_print_time).count() >= 2) {
            last_print_time = now;

            std::lock_guard<std::mutex> lock(g_apps_mutex);
            if (!g_apps.empty()) {
                // 统计真正存活的数量
                int alive_count = 0;
                for (auto& app : g_apps) {
                    if (app->IsRunning()) alive_count++;
                }

                std::cout << "\n=== [后台监控] 当前集群总规模: " << g_apps.size()
                    << " 架 | 存活: " << alive_count << " 架 ===" << std::endl;

                int print_count = g_apps.size() > 3 ? 3 : g_apps.size();
                for (int i = 0; i < print_count; ++i) {
                    g_apps[i]->PrintDebugStatus();
                }
            }
        }

        // 每次循环稍微休息一下，防止打满 CPU 单核
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    SwarmComm::ShutdownDDS();
    return 0;
}