#include "DroneStateCache.h"

void DroneStateCache::updateState(const DroneState& state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_states[state.drone_id] = state;
}

std::optional<DroneState> DroneStateCache::getState(uint32_t droneId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_states.find(droneId);
    if (it != m_states.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<DroneState> DroneStateCache::getAllStates() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<DroneState> result;
    result.reserve(m_states.size());
    for (const auto& pair : m_states) {
        result.push_back(pair.second);
    }
    return result;
}

size_t DroneStateCache::getDroneCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_states.size();
}

void DroneStateCache::removeState(uint32_t droneId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_states.erase(droneId);
}

void DroneStateCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_states.clear();
}