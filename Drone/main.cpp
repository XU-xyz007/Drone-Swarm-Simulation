#include "DroneApp.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "FormationAlgorithm.h"

std::vector<std::shared_ptr<DroneApp>> g_apps;
std::vector<std::thread> g_threads;
std::mutex g_apps_mutex;

int main(int argc, char* argv[]) {
    std::cout << "========== Drone swarm node manager ==========" << std::endl;
    std::cout << "-> Initializing DDS..." << std::endl;

    if (!SwarmComm::InitDDS()) {
        std::cerr << "ZR-DDS initialization failed. Check runtime files next to the executable." << std::endl;
        return 1;
    }

    SwarmComm::SubscribeCmd([](const SharedMessage::DroneCmd& cmd) {
        static std::atomic<bool> firstContact(false);
        if (!firstContact.exchange(true)) {
            std::cout << "[DDS] First command received: cmd_id=" << cmd.cmd_id
                      << ", expected_count=" << cmd.expected_count << std::endl;
        }

        std::lock_guard<std::mutex> lock(g_apps_mutex);
        if (cmd.cmd_type == SharedMessage::CMD_TAKEOFF || cmd.cmd_type == SharedMessage::CMD_FORMATION) {
            const int targetCount = static_cast<int>(cmd.expected_count);
            const int currentCount = static_cast<int>(g_apps.size());
            if (targetCount > currentCount) {
                SwarmFormation::FormationRequest initRequest;
                initRequest.formation_id = 0;
                initRequest.formation_text = "";
                initRequest.count = targetCount;
                initRequest.center_x = 0.0;
                initRequest.center_y = -50.0;
                initRequest.spacing = cmd.spacing > 0.0 ? cmd.spacing : 2.0;
                const SwarmFormation::FormationResult initialSlots =
                    SwarmFormation::GenerateSlots(initRequest);

                for (int i = currentCount; i < targetCount; ++i) {
                    double initialX = 0.0;
                    double initialY = -50.0;
                    if (initialSlots.success && static_cast<size_t>(i) < initialSlots.slots.size()) {
                        initialX = initialSlots.slots[static_cast<size_t>(i)].x;
                        initialY = initialSlots.slots[static_cast<size_t>(i)].y;
                    }
                    const unsigned long droneId = static_cast<unsigned long>(i + 1);
                    std::shared_ptr<DroneApp> app(new DroneApp(droneId, initialX, initialY));
                    g_apps.push_back(app);
                    g_threads.emplace_back(&DroneApp::Start, app.get());
                    app->OnCommandReceived(cmd);
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
            }
        }

        for (std::vector<std::shared_ptr<DroneApp>>::iterator it = g_apps.begin(); it != g_apps.end(); ++it) {
            (*it)->OnCommandReceived(cmd);
        }
    });

    SwarmComm::SubscribeState([](const SharedMessage::DroneState& state) {
        std::lock_guard<std::mutex> lock(g_apps_mutex);
        for (std::vector<std::shared_ptr<DroneApp>>::iterator it = g_apps.begin(); it != g_apps.end(); ++it) {
            (*it)->OnStateReceived(state);
        }
    });

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::lock_guard<std::mutex> lock(g_apps_mutex);
        if (!g_apps.empty()) {
            std::cout << "[Swarm] Active drones: " << g_apps.size() << std::endl;
            const size_t printCount = g_apps.size() < 3 ? g_apps.size() : 3;
            for (size_t i = 0; i < printCount; ++i) g_apps[i]->PrintDebugStatus();
        }
    }
}
