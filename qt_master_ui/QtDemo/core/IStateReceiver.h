#pragma once
#include "DroneState.h"
#include <functional>
#include <cstdint>

class IStateReceiver {
public:
    virtual ~IStateReceiver() = default;

    virtual void setOnStateReceived(
        std::function<void(const DroneState&)> callback) = 0;

    virtual void setOnDroneRemoved(
        std::function<void(uint32_t)> callback) {}

    virtual void start() = 0;

    virtual void stop() = 0;

    virtual void setLatestCmdId(uint32_t cmdId) {}

    virtual void setDroneCount(uint32_t count) {}
};