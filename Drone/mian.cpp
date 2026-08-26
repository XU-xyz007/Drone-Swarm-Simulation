#include "drone_msg.h"
#include "drone_msgTypeSupport.h"
#include "drone_msgDataReader.h"
#include "ZRDDSCppSimpleInterface.h"
#include <stdio.h>

using namespace DDS;

class DroneCmdListener : public SimpleDataReaderListener<DroneCmd, DroneCmdSeq, DroneCmdDataReader> {
public:
    virtual void on_process_sample(DataReader* reader, const DroneCmd& sample, const SampleInfo& info) {
        printf("【收到指令】无人机 %d 飞往 X=%.1f, Y=%.1f\n", sample.drone_id, sample.target_x, sample.target_y);
    }
};

DroneCmdListener g_listener;

int main() {
    DomainParticipantFactory* factory = DDSIF::Init(NULL, "non_rio");
    DomainParticipant* udpDp = DDSIF::CreateDP(150, "udp_dp");
    if (NULL == udpDp) { printf("DDS Init Failed!\n"); return -1; }

    DataReader* reader = DDSIF::SubTopic(udpDp, "UAV_Command", DroneCmdTypeSupport::get_instance(), "non_zerocopy_reliable", &g_listener);

    printf("无人机待命...\n");
    while (true) { ZRSleep(1000); }

    DDSIF::Finalize();
    return 0;
}