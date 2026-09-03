#include <windows.h>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "../Communication/DdsService.h"

namespace {

const unsigned long kMovingPeriodMs = 100;  // 10 Hz
const unsigned long kStaticPeriodMs = 1000; // 1 Hz
const double kBoundX = 60.0;
const double kBoundY = 40.0;

struct SimDrone {
    SharedMessage::DroneState state;
    bool movesAfterCommand;
    double vx;
    double vy;
    unsigned long long lastPublishMs;
    unsigned long long lastMoveMs;
};

std::atomic<DDS_ULong> gCurrentCmdId(0);
std::atomic<bool> gCommandActive(false);
std::atomic<bool> gPrintedStateCallback(false);

int ReadInt(const char* value, int fallback, int minValue, int maxValue)
{
    if (value == NULL) return fallback;
    const int parsed = std::atoi(value);
    if (parsed < minValue) return minValue;
    if (parsed > maxValue) return maxValue;
    return parsed;
}

double ReadDouble(const char* value, double fallback, double minValue, double maxValue)
{
    if (value == NULL) return fallback;
    const double parsed = std::atof(value);
    if (parsed < minValue) return minValue;
    if (parsed > maxValue) return maxValue;
    return parsed;
}

void UpdatePosition(SimDrone& drone, unsigned long long nowMs)
{
    const double elapsedSeconds = static_cast<double>(nowMs - drone.lastMoveMs) / 1000.0;
    drone.lastMoveMs = nowMs;
    drone.state.x += drone.vx * elapsedSeconds;
    drone.state.y += drone.vy * elapsedSeconds;

    // Deterministic bounce movement: easy to see in A's Qt scene and bounded
    // so a long pressure run does not make the map grow indefinitely.
    if (drone.state.x > kBoundX || drone.state.x < -kBoundX) {
        drone.vx = -drone.vx;
        drone.state.x = drone.state.x > kBoundX ? kBoundX : -kBoundX;
    }
    if (drone.state.y > kBoundY || drone.state.y < -kBoundY) {
        drone.vy = -drone.vy;
        drone.state.y = drone.state.y > kBoundY ? kBoundY : -kBoundY;
    }
}

} // namespace

// One process simulates N Drone nodes. It is a DDS pressure-data producer for
// the Controller UI: moving nodes publish changing coordinates at 10 Hz while
// static nodes publish at 1 Hz. It is deliberately independent from C's final
// motion implementation.
//
// Usage: Drone.exe [count] [movingCount] [speedMps] [durationSeconds]
// Example: Drone.exe 99 50 3 60
int main(int argc, char** argv)
{
    const int droneCount = argc >= 2 ? ReadInt(argv[1], 5, 1, 99) : 5;
    const int movingCount = argc >= 3 ? ReadInt(argv[2], droneCount, 0, droneCount) : droneCount;
    const double speedMps = argc >= 4 ? ReadDouble(argv[3], 3.0, 0.1, 30.0) : 3.0;
    const int durationSeconds = argc >= 5 ? ReadInt(argv[4], 0, 0, 3600) : 0;

    printf("========== FakeDrone DDS load-test producer ==========\n");
    printf("Nodes: %d | moving after command: %d (10 Hz) | static: %d (1 Hz) | speed: %.1f m/s\n",
           droneCount, movingCount, droneCount - movingCount, speedMps);
    printf("Duration: %s. Ctrl+C to stop.\n", durationSeconds == 0 ? "unlimited" : "limited");

    SwarmComm::SubscribeCmd([](const SharedMessage::DroneCmd& cmd) {
        const DDS_ULong previous = gCurrentCmdId.exchange(cmd.cmd_id);
        if (previous == cmd.cmd_id) {
            printf("[FakeDrone] duplicate cmd_id=%u ignored.\n", (unsigned)cmd.cmd_id);
            return;
        }
        gCommandActive.store(true);
        printf("[FakeDrone] command accepted: cmd_id=%u, type=%u, formation=%u, text=%s\n",
               (unsigned)cmd.cmd_id, (unsigned)cmd.cmd_type, (unsigned)cmd.formation_id,
               cmd.formation_text == NULL ? "" : cmd.formation_text);
    });
    SwarmComm::SubscribeState([](const SharedMessage::DroneState& state) {
        bool expected = false;
        if (gPrintedStateCallback.compare_exchange_strong(expected, true)) {
            printf("[FakeDrone] state reader is active (first state: drone %u).\n",
                   (unsigned)state.drone_id);
        }
    });
    if (!SwarmComm::InitDDS()) {
        printf("InitDDS failed. Ensure ZRDDS_QOS_PROFILES.xml and zrddslicence.lic are beside the executable.\n");
        return 1;
    }

    const unsigned long long startMs = GetTickCount64();
    std::vector<SimDrone> drones(static_cast<size_t>(droneCount));
    for (int i = 0; i < droneCount; ++i) {
        SimDrone& drone = drones[static_cast<size_t>(i)];
        SharedMessage::DroneStateInitialize(&drone.state);
        drone.state.drone_id = static_cast<DDS_ULong>(i + 1);
        drone.state.status = SharedMessage::ST_IDLE;
        drone.state.x = (i % 11) * 8.0 - 40.0;
        drone.state.y = (i / 11) * 8.0 - 32.0;
        drone.movesAfterCommand = i < movingCount;

        const double angle = 0.55 * static_cast<double>(i + 1);
        drone.vx = speedMps * std::cos(angle);
        drone.vy = speedMps * std::sin(angle);
        drone.lastPublishMs = 0;
        drone.lastMoveMs = startMs;
    }

    unsigned long long nextStatsMs = startMs + 1000;
    unsigned long long publishCount = 0;
    while (true) {
        const unsigned long long nowMs = GetTickCount64();
        if (durationSeconds > 0 && nowMs - startMs >= static_cast<unsigned long long>(durationSeconds) * 1000ULL) {
            break;
        }

        const DDS_ULong cmdId = gCurrentCmdId.load();
        const bool commandActive = gCommandActive.load();
        for (size_t i = 0; i < drones.size(); ++i) {
            SimDrone& drone = drones[i];
            const bool moving = commandActive && drone.movesAfterCommand;
            const unsigned long periodMs = moving ? kMovingPeriodMs : kStaticPeriodMs;
            if (drone.lastPublishMs != 0 && nowMs - drone.lastPublishMs < periodMs) {
                continue;
            }

            if (moving) {
                UpdatePosition(drone, nowMs);
                drone.state.status = SharedMessage::ST_CRUISE;
            } else {
                drone.lastMoveMs = nowMs;
                drone.state.status = SharedMessage::ST_IDLE;
            }
            drone.state.current_cmd_id = cmdId;
            SwarmComm::PublishState(drone.state);
            drone.lastPublishMs = nowMs;
            ++publishCount;
        }

        if (nowMs >= nextStatsMs) {
            printf("[FakeDrone] %llu state samples published in last second | moving=%d@10Hz static=%d@1Hz cmd_id=%u\n",
                   publishCount, commandActive ? movingCount : 0,
                   commandActive ? droneCount - movingCount : droneCount, (unsigned)cmdId);
            publishCount = 0;
            nextStatsMs += 1000;
        }
        ZRSleep(10);
    }

    SwarmComm::ShutdownDDS();
    printf("FakeDrone load test finished.\n");
    return 0;
}
