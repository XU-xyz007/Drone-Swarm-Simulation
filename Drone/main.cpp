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
constexpr unsigned long kMaximumDroneCount = 999;

std::shared_ptr<DroneApp> CreateDroneApp(unsigned long droneId,
                                         const SharedMessage::DroneCmd& cmd) {
    SwarmFormation::FormationRequest initReq;
    initReq.formation_id = 0;
    initReq.formation_text = "";
    initReq.count = cmd.expected_count;
    initReq.center_x = 0.0;
    initReq.center_y = -50.0;
    initReq.spacing = cmd.spacing > 0.0 ? cmd.spacing : 2.0;

    const auto initResult = SwarmFormation::GenerateSlots(initReq);
    double initX = 0.0;
    double initY = -50.0;
    const size_t index = static_cast<size_t>(droneId - 1);
    if (initResult.success && index < initResult.slots.size()) {
        initX = initResult.slots[index].x;
        initY = initResult.slots[index].y;
    }

    auto app = std::make_shared<DroneApp>(droneId, initX, initY);
    // Capture the shared_ptr so retired workers can finish safely if an ID is reused.
    g_threads.emplace_back([app] { app->Start(); });
    return app;
}

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
        static std::atomic<bool> first_contact{ false };
        if (!first_contact.exchange(true)) {
            std::cout << "\n[System] command link ready: cmd_id=" << cmd.cmd_id
                      << " | target=" << cmd.expected_count << std::endl;
        }

        std::vector<std::shared_ptr<DroneApp>> appsToCommand;
        std::vector<std::shared_ptr<DroneApp>> appsToLand;
        std::vector<std::shared_ptr<DroneApp>> appsToCancelLanding;
        if (cmd.cmd_type == SharedMessage::CMD_TAKEOFF ||
            cmd.cmd_type == SharedMessage::CMD_FORMATION) {
            const unsigned long targetCount = std::min<unsigned long>(
                cmd.expected_count, kMaximumDroneCount);

            {
                std::lock_guard<std::mutex> lock(g_apps_mutex);
                if (g_apps.size() < targetCount) g_apps.resize(targetCount);

                // IDs 1..targetCount remain active. Recreate only instances
                // that already completed a planned landing and exited.
                for (unsigned long id = 1; id <= targetCount; ++id) {
                    auto& app = g_apps[id - 1];
                    if (!app || !app->IsRunning()) {
                        app = CreateDroneApp(id, cmd);
                    } else if (app->IsLanding()) {
                        appsToCancelLanding.push_back(app);
                    }
                    appsToCommand.push_back(app);
                }

                // Scale-down: higher IDs land visually and do not receive
                // the new formation command.
                for (size_t index = targetCount; index < g_apps.size(); ++index) {
                    if (g_apps[index] && g_apps[index]->IsRunning() &&
                        !g_apps[index]->IsLanding()) {
                        appsToLand.push_back(g_apps[index]);
                    }
                }
            }

            for (const auto& app : appsToCancelLanding) app->CancelLanding();
            for (const auto& app : appsToLand) app->BeginLanding();
            for (const auto& app : appsToCommand) app->OnCommandReceived(cmd);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(g_apps_mutex);
            for (const auto& app : g_apps) {
                if (app && app->IsRunning()) appsToCommand.push_back(app);
            }
        }

        // Landing reaches every currently active drone, regardless of the UI N field.
        for (const auto& app : appsToCommand) app->OnCommandReceived(cmd);
    });
    // ================= 2. 订阅邻居状态更新 =================
    SwarmComm::SubscribeState([](const SharedMessage::DroneState& state) {
        std::vector<std::shared_ptr<DroneApp>> activeApps;
        {
            std::lock_guard<std::mutex> lock(g_apps_mutex);
            for (const auto& app : g_apps) {
                if (app && app->IsRunning() && !app->IsLanding()) {
                    activeApps.push_back(app);
                }
            }
        }
        for (const auto& app : activeApps) app->OnStateReceived(state);
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