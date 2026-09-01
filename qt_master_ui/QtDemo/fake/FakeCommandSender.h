#pragma once
#include "core/ICommandSender.h"
#include <cstdint>
#include <string>

class FakeCommandSender : public ICommandSender {
public:
    bool sendCommand(const DroneCmd& cmd) override;
    uint32_t getLastCmdId() const override;

private:
    uint32_t m_cmdCounter = 0;
};