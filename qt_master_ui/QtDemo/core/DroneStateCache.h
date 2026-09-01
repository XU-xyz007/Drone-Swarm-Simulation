#pragma once
#include "IDroneStateCache.h"
#include <unordered_map>

class DroneStateCache : public IDroneStateCache {
public:
    void updateState(const DroneState& state) override;
    std::optional<DroneState> getState(uint32_t droneId) const override;
    std::vector<DroneState> getAllStates() const override;
    size_t getDroneCount() const override;
    void removeState(uint32_t droneId) override;
    void clear() override;

private:
    std::unordered_map<uint32_t, DroneState> m_states;
};