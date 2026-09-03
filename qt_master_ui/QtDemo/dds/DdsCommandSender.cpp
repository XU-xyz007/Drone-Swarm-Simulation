#include "DdsCommandSender.h"

#include "../../../Communication/DdsService.h"

#include <cstring>
#include <iostream>

bool DdsCommandSender::sendCommand(const DroneCmd& cmd)
{
    SharedMessage::DroneCmd nativeCmd;
    SharedMessage::DroneCmdInitialize(&nativeCmd);
    nativeCmd.cmd_id = static_cast<DDS_ULong>(cmd.cmd_id);
    nativeCmd.cmd_type = static_cast<SharedMessage::DroneCmdType>(cmd.cmd_type);
    nativeCmd.formation_id = static_cast<DDS_ULong>(cmd.formation_id);
    strcpy_s(nativeCmd.formation_text, 256, cmd.formation_text.c_str());
    nativeCmd.target_x = cmd.target_x;
    nativeCmd.target_y = cmd.target_y;
    nativeCmd.spacing = cmd.spacing;
    nativeCmd.expected_count = static_cast<DDS_ULong>(cmd.expected_count);

    const bool ok = SwarmComm::PublishCmd(nativeCmd);
    SharedMessage::DroneCmdFinalize(&nativeCmd);
    if (ok) m_lastCmdId = cmd.cmd_id;

    std::cout << "[DDS-UI] PublishCmd cmd_id=" << cmd.cmd_id
              << (ok ? " OK" : " FAILED") << std::endl;
    return ok;
}

uint32_t DdsCommandSender::getLastCmdId() const
{
    return m_lastCmdId;
}
