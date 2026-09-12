#pragma once
#include "DroneState.h"
#include <functional>
#include <cstdint>

struct CommunicationTelemetry {
    uint64_t stateSamplesPerSecond = 0;
    uint64_t payloadBytesPerSecond = 0;
    uint64_t sequenceGaps = 0;
    uint64_t outOfOrderSamples = 0;
    bool available = false;
};

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

    // 取出并清零最近一个一秒窗口的主控端通信统计。
    virtual CommunicationTelemetry takeCommunicationTelemetry() {
        return CommunicationTelemetry();
    }
};
