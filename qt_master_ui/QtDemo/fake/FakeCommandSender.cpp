#include "FakeCommandSender.h"
#include <iostream>

bool FakeCommandSender::sendCommand(const DroneCmd& cmd) {
    m_cmdCounter = cmd.cmd_id;
    std::cout << "[Fake] 发送命令: cmd_id=" << cmd.cmd_id
              << ", type=" << static_cast<uint32_t>(cmd.cmd_type)
              << ", formation_id=" << cmd.formation_id
              << ", text=\"" << cmd.formation_text << "\""
              << ", target=(" << cmd.target_x << "," << cmd.target_y << ")"
              << ", spacing=" << cmd.spacing
              << ", N=" << cmd.expected_count
              << std::endl;
    return true;
}

uint32_t FakeCommandSender::getLastCmdId() const {
    return m_cmdCounter;
}