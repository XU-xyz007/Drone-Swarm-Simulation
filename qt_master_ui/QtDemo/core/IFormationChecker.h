#pragma once
#include <cstdint>
#include <utility>
#include "DroneState.h"

class IFormationChecker {
public:
    virtual ~IFormationChecker() = default;

    virtual void setExpectedCount(uint32_t count) = 0;

    virtual void setLatestCmdId(uint32_t cmdId) = 0;

    virtual void updateDroneCmdId(uint32_t droneId, uint32_t currentCmdId) = 0;

    virtual void updateDroneStatus(uint32_t droneId, DroneStatus status) = 0;

    virtual bool isFormationComplete() const = 0;

    virtual std::pair<size_t, size_t> getProgress() const = 0;

    virtual void reset() = 0;
};