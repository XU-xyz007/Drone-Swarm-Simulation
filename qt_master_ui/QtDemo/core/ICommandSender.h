#pragma once
#include "DroneCmd.h"
#include <cstdint>

class ICommandSender {
public:
    virtual ~ICommandSender() = default;

    virtual bool sendCommand(const DroneCmd& cmd) = 0;

    virtual uint32_t getLastCmdId() const = 0;
};