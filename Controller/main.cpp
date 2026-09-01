#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>

#include "../Communication/DdsService.h"

namespace {

std::mutex gStateMutex;
std::map<DDS_ULong, SharedMessage::DroneState> gLatestStates;

void PrintSummary()
{
    std::lock_guard<std::mutex> lock(gStateMutex);
    printf("[FakeController] received state from %u drone(s):", (unsigned)gLatestStates.size());
    for (std::map<DDS_ULong, SharedMessage::DroneState>::const_iterator it = gLatestStates.begin();
         it != gLatestStates.end(); ++it) {
        printf(" #%u(%.1f,%.1f,cmd=%u)", (unsigned)it->second.drone_id,
               it->second.x, it->second.y, (unsigned)it->second.current_cmd_id);
    }
    printf("\n");
}

} // namespace

int main(int argc, char** argv)
{
    const int expectedCount = argc >= 2 ? (std::atoi(argv[1]) > 0 ? std::atoi(argv[1]) : 5) : 5;
    printf("========== FakeController (DDS command + state callback) ==========\n");

    SwarmComm::SubscribeState([](const SharedMessage::DroneState& state) {
        std::lock_guard<std::mutex> lock(gStateMutex);
        gLatestStates[state.drone_id] = state;
    });
    if (!SwarmComm::InitDDS()) {
        printf("InitDDS failed. Ensure ZRDDS_QOS_PROFILES.xml and zrddslicence.lic are beside the executable.\n");
        return 1;
    }

    SharedMessage::DroneCmd cmd;
    SharedMessage::DroneCmdInitialize(&cmd);
    cmd.cmd_id = 1;
    cmd.cmd_type = SharedMessage::CMD_FORMATION;
    cmd.formation_id = 1;
    strcpy_s(cmd.formation_text, 256, "LINE");
    cmd.target_x = 0.0;
    cmd.target_y = 0.0;
    cmd.spacing = 3.0;
    cmd.expected_count = static_cast<DDS_ULong>(expectedCount);

    printf("Waiting 1 second for DDS discovery, then broadcasting cmd_id=1 to %d drones.\n", expectedCount);
    ZRSleep(1000);
    for (int send = 1; send <= 3; ++send) {
        const bool ok = SwarmComm::PublishCmd(cmd);
        printf("[FakeController] PublishCmd #%d: %s\n", send, ok ? "OK" : "FAILED");
        ZRSleep(500);
    }
    for (int second = 1; second <= 8; ++second) {
        ZRSleep(1000);
        PrintSummary();
    }

    SharedMessage::DroneCmdFinalize(&cmd);
    SwarmComm::ShutdownDDS();
    printf("FakeController finished.\n");
    return 0;
}
