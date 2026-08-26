#include "drone_msg.h" 
#include "drone_msgTypeSupport.h"
#include "drone_msgDataWriter.h"
#include "ZRDDSCppSimpleInterface.h"
#include <stdio.h>

using namespace DDS;

int main() {
    // 1. 初始化引擎
    DomainParticipantFactory* factory = DDSIF::Init(NULL, "non_rio");
    DomainParticipant* udpDp = DDSIF::CreateDP(150, "udp_dp");
    if (NULL == udpDp) { printf("DDS Init Failed!\n"); return -1; }

    // 2. 创建发送端（注意看这里！我补上了第 5 个参数 NULL，解决了 E0165 报错）
    DataWriter* writer = DDSIF::PubTopic(udpDp, "UAV_Command", DroneCmdTypeSupport::get_instance(), "non_zerocopy_reliable", NULL);

    // 3. 类型转换（注意看这里！我去掉了 narrow，改成了直接强转，解决了 E0135 报错）
    DroneCmdDataWriter* cmd_writer = (DroneCmdDataWriter*)writer;

    // 4. 业务逻辑
    DroneCmd my_cmd;
    my_cmd.drone_id = 1;
    my_cmd.target_x = 100.0;
    my_cmd.target_y = 200.0;

    printf("主控台启动，准备发送...\n");
    for (int i = 0; i < 20; i++) {
        my_cmd.target_x += 1.5;

        // ZRDDS 发送动作
        cmd_writer->write(my_cmd, HANDLE_NIL_NATIVE);

        printf("【发送】无人机 %d, X=%.1f, Y=%.1f\n", my_cmd.drone_id, my_cmd.target_x, my_cmd.target_y);
        ZRSleep(1000);
    }

    DDSIF::Finalize();
    return 0;
}