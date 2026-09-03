#pragma once

#include "core/ICommandSender.h"

// Qt UI 到 ZR-DDS 命令 Topic 的适配器。它只做格式转换，不参与编队算法。
class DdsCommandSender : public ICommandSender {
public:
    bool sendCommand(const DroneCmd& cmd) override;
    uint32_t getLastCmdId() const override;

private:
    uint32_t m_lastCmdId = 0;
};
