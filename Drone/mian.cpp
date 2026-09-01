#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "../Communication/DdsService.h"

namespace {

std::atomic<DDS_ULong> gCurrentCmdId(0);
std::atomic<unsigned long long> gMovingUntilMs(0);
std::atomic<bool> gPrintedStateCallback(false);

} // namespace

// One process simulates N Drone nodes. This avoids launching N Windows
// processes when A needs 64 or 99 state samples for UI stress testing.
int main(int argc, char** argv)
{
    int droneCount = argc >= 2 ? std::atoi(argv[1]) : 5;
    if (droneCount < 1) droneCount = 1;
    if (droneCount > 99) droneCount = 99;

    printf("========== FakeDrone (DDS command callback + 10 Hz states) ==========\n");
    printf("Simulating %d DroneState publishers in one process. Ctrl+C to stop.\n", droneCount);

    SwarmComm::SubscribeCmd([](const SharedMessage::DroneCmd& cmd) {
        const DDS_ULong previous = gCurrentCmdId.exchange(cmd.cmd_id);
        // Fake a three-second formation transition.  A real Drone's C-side
        // motion state machine decides this interval from its own target error.
        gMovingUntilMs.store(GetTickCount64() + 3000ULL);
        printf("[FakeDrone] command callback: cmd_id=%u, formation=%u, text=%s, previous=%u\n",
               (unsigned)cmd.cmd_id, (unsigned)cmd.formation_id,
               cmd.formation_text == NULL ? "" : cmd.formation_text, (unsigned)previous);
    });
    SwarmComm::SubscribeState([](const SharedMessage::DroneState& state) {
        bool expected = false;
        if (gPrintedStateCallback.compare_exchange_strong(expected, true)) {
            printf("[FakeDrone] state callback is active (first state: drone %u).\n", (unsigned)state.drone_id);
        }
    });
    if (!SwarmComm::InitDDS()) {
        printf("InitDDS failed. Ensure ZRDDS_QOS_PROFILES.xml and zrddslicence.lic are beside the executable.\n");
        return 1;
    }

    std::vector<SharedMessage::DroneState> states(static_cast<size_t>(droneCount));
    for (int i = 0; i < droneCount; ++i) {
        SharedMessage::DroneStateInitialize(&states[static_cast<size_t>(i)]);
        states[static_cast<size_t>(i)].drone_id = static_cast<DDS_ULong>(i + 1);
        states[static_cast<size_t>(i)].status = SharedMessage::ST_IDLE;
        states[static_cast<size_t>(i)].x = (i % 10) * 3.0;
        states[static_cast<size_t>(i)].y = (i / 10) * 3.0;
    }

    while (true) {
        const DDS_ULong cmdId = gCurrentCmdId.load();
        const bool moving = cmdId != 0 && GetTickCount64() < gMovingUntilMs.load();
        // C owns the tick schedule: 10 Hz while moving, 1 Hz while static.
        // DdsService does not impose a rate or interpret a slower state stream
        // as a disconnect.
        const unsigned long periodMs = moving ? 100 : 1000;
        for (size_t i = 0; i < states.size(); ++i) {
            SharedMessage::DroneState& state = states[i];
            state.current_cmd_id = cmdId;
            state.status = cmdId == 0 ? SharedMessage::ST_IDLE : SharedMessage::ST_CRUISE;
            SwarmComm::PublishState(state);
        }
        printf("[FakeDrone] published %d states at %lu Hz mode, current_cmd_id=%u\n",
               droneCount, moving ? 10UL : 1UL, (unsigned)cmdId);
        ZRSleep(periodMs);
    }
}
