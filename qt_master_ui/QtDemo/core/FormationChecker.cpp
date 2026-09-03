#include "FormationChecker.h"
#include <iostream>

void FormationChecker::setExpectedCount(uint32_t count) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_expectedCount = count;
}

void FormationChecker::setLatestCmdId(uint32_t cmdId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_latestCmdId = cmdId;
}

void FormationChecker::updateDroneCmdId(uint32_t droneId, uint32_t currentCmdId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_droneCmdIds[droneId] = currentCmdId;
}

void FormationChecker::updateDroneStatus(uint32_t droneId, DroneStatus status) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_droneStatuses[droneId] = status;
}

size_t FormationChecker::countIdleLocked() const {
    size_t count = 0;
    for (const auto& pair : m_droneStatuses) {
        if (pair.second == DroneStatus::ST_IDLE) {
            ++count;
        }
    }
    return count;
}

bool FormationChecker::isFormationComplete() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_expectedCount == 0) return false;

    if (countIdleLocked() < m_expectedCount) return false;

    for (const auto& pair : m_droneCmdIds) {
        if (pair.second != m_latestCmdId) return false;
    }

    return true;
}

std::pair<size_t, size_t> FormationChecker::getProgress() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return { countIdleLocked(), m_expectedCount };
}

void FormationChecker::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_droneCmdIds.clear();
    m_droneStatuses.clear();
}