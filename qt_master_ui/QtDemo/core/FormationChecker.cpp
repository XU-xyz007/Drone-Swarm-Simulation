#include "FormationChecker.h"
#include <iostream>

void FormationChecker::setExpectedCount(uint32_t count) {
    m_expectedCount = count;
}

void FormationChecker::setLatestCmdId(uint32_t cmdId) {
    m_latestCmdId = cmdId;
}

void FormationChecker::updateDroneCmdId(uint32_t droneId, uint32_t currentCmdId) {
    m_droneCmdIds[droneId] = currentCmdId;
}

void FormationChecker::updateDroneStatus(uint32_t droneId, DroneStatus status) {
    auto it = m_droneStatuses.find(droneId);
    DroneStatus prev = (it != m_droneStatuses.end()) ? it->second : DroneStatus::ST_IDLE;

    m_droneStatuses[droneId] = status;

    // 检测 ST_CRUISE → ST_IDLE 过渡
    if (prev == DroneStatus::ST_CRUISE && status == DroneStatus::ST_IDLE) {
        m_droneTransitioned.insert(droneId);
        std::cout << "[判定] 无人机 " << droneId << " 完成过渡 ST_CRUISE→ST_IDLE"
                  << " (已过渡: " << m_droneTransitioned.size() << "/" << m_expectedCount << ")"
                  << std::endl;
    }
}

bool FormationChecker::isFormationComplete() const {
    if (m_expectedCount == 0) return false;

    // 条件 1：所有 N 架无人机都完成了 ST_CRUISE → ST_IDLE 过渡
    if (m_droneTransitioned.size() < m_expectedCount) return false;

    // 条件 2：所有无人机的 current_cmd_id 等于最新 cmd_id
    for (const auto& pair : m_droneCmdIds) {
        if (pair.second != m_latestCmdId) return false;
    }

    return true;
}

std::pair<size_t, size_t> FormationChecker::getProgress() const {
    return { m_droneTransitioned.size(), m_expectedCount };
}

void FormationChecker::reset() {
    m_droneCmdIds.clear();
    m_droneStatuses.clear();
    m_droneTransitioned.clear();
}