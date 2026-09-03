#pragma once
#include "DroneState.h"
#include <optional>
#include <vector>

class IDroneStateCache {
public:
    virtual ~IDroneStateCache() = default;

    virtual void updateState(const DroneState& state) = 0;

    virtual std::optional<DroneState> getState(uint32_t droneId) const = 0;

    virtual std::vector<DroneState> getAllStates() const = 0;

    virtual size_t getDroneCount() const = 0;

    virtual void removeState(uint32_t droneId) = 0;

    virtual void clear() = 0;
};