#pragma once
#include "IFormationChecker.h"
#include <unordered_map>
#include <mutex>
#include <cstdint>

class FormationChecker : public IFormationChecker {
public:
    void setExpectedCount(uint32_t count) override;
    void setLatestCmdId(uint32_t cmdId) override;
    void updateDroneCmdId(uint32_t droneId, uint32_t currentCmdId) override;
    void updateDroneStatus(uint32_t droneId, DroneStatus status) override;
    bool isFormationComplete() const override;
    std::pair<size_t, size_t> getProgress() const override;
    void reset() override;

private:
    size_t countIdleLocked() const;

    uint32_t m_expectedCount = 0;
    uint32_t m_latestCmdId = 0;
    std::unordered_map<uint32_t, uint32_t> m_droneCmdIds;
    std::unordered_map<uint32_t, DroneStatus> m_droneStatuses;
    mutable std::mutex m_mutex;
};