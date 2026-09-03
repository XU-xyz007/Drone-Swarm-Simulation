#include "FakeStateReceiver.h"
#include <random>
#include <algorithm>

FakeStateReceiver::FakeStateReceiver(QObject* parent)
    : QObject(parent)
    , m_rng(std::random_device{}())
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &FakeStateReceiver::onTimerTick);
}

void FakeStateReceiver::setOnStateReceived(std::function<void(const DroneState&)> callback) {
    m_callback = callback;
}

void FakeStateReceiver::setOnDroneRemoved(std::function<void(uint32_t)> callback) {
    m_removeCallback = callback;
}

void FakeStateReceiver::start() {
    if (m_droneCount == 0) return;
    m_timer->start(100);  // 10Hz = 100ms
}

void FakeStateReceiver::stop() {
    m_timer->stop();
}

void FakeStateReceiver::setDroneCount(uint32_t count) {
    if (count == m_userCount) return;

    if (count < m_userCount) {
        m_userCount = count;
        for (uint32_t i = count; i < m_droneCount; ++i) {
            m_flyOutDrone[i] = true;
        }
        return;
    }

    uint32_t oldUserCount = m_userCount;
    uint32_t oldDroneCount = m_droneCount;
    m_userCount = count;
    m_droneCount = count;
    m_x.resize(count, 0.0);
    m_y.resize(count, 0.0);
    m_droneCmdIds.resize(count, 0);
    m_flyOutDrone.resize(count, false);
    m_flyInDrone.resize(count, false);
    m_removed.resize(count, false);

    if (count > oldDroneCount) {
        std::uniform_real_distribution<double> distY(-30.0, 30.0);
        for (uint32_t i = oldDroneCount; i < count; ++i) {
            m_x[i] = -FLY_IN_OFFSET;
            m_y[i] = distY(m_rng);
            m_flyInDrone[i] = true;
        }
    } else if (count > oldUserCount) {
        for (uint32_t i = oldUserCount; i < count; ++i) {
            m_flyOutDrone[i] = false;
            m_removed[i] = false;
            m_flyInDrone[i] = true;
            m_x[i] = -FLY_IN_OFFSET;
        }
    }
}

void FakeStateReceiver::setLatestCmdId(uint32_t cmdId) {
    m_latestCmdId = cmdId;
}

void FakeStateReceiver::applySeparation() {
    const double iterStrength = SEPARATION_STRENGTH / SEPARATION_ITERATIONS;
    for (int iter = 0; iter < SEPARATION_ITERATIONS; ++iter) {
        for (uint32_t i = 0; i < m_droneCount; ++i) {
            if (m_flyOutDrone[i] || m_removed[i]) continue;
            double sepX = 0.0, sepY = 0.0;
            for (uint32_t j = 0; j < m_droneCount; ++j) {
                if (i == j || m_flyOutDrone[j] || m_removed[j]) continue;
                double dx = m_x[i] - m_x[j];
                double dy = m_y[i] - m_y[j];
                double dist = std::sqrt(dx * dx + dy * dy);
                if (dist < MIN_SEPARATION && dist > 0.001) {
                    double force = iterStrength * (MIN_SEPARATION - dist) / MIN_SEPARATION;
                    sepX += dx / dist * force;
                    sepY += dy / dist * force;
                }
            }
            m_x[i] += sepX;
            m_y[i] += sepY;
        }
    }
}

void FakeStateReceiver::onTimerTick() {
    if (!m_callback || m_droneCount == 0) return;

    std::uniform_real_distribution<double> stepDist(-0.5, 0.5);
    ++m_tickCount;
    const double speed = 3.0;
    const double flyOutEndX = FLY_OUT_OFFSET;

    for (uint32_t i = 0; i < m_droneCount; ++i) {
        if (m_removed[i]) {
            continue;
        }

        if (m_flyOutDrone[i]) {
            double dx = flyOutEndX - m_x[i];
            if (std::abs(dx) < 2.0) {
                m_x[i] = flyOutEndX;
                if (m_removeCallback) {
                    m_removeCallback(i + 1);
                }
                m_flyOutDrone[i] = false;
                m_removed[i] = true;
            } else {
                m_x[i] += (dx > 0 ? 1.0 : -1.0) * speed;
            }
            continue;
        }

        if (m_flyInDrone[i]) {
            double dx = 0.0 - m_x[i];
            if (std::abs(dx) < 2.0) {
                m_x[i] = 0.0;
                m_flyInDrone[i] = false;
            } else {
                m_x[i] += (dx > 0 ? 1.0 : -1.0) * speed;
            }
            continue;
        }

        m_x[i] += stepDist(m_rng);
        m_y[i] += stepDist(m_rng);
    }

    applySeparation();

    bool updateCmdId = (m_tickCount % 5 == 0);
    std::uniform_int_distribution<uint32_t> droneDist(0, m_droneCount - 1);

    for (uint32_t i = 0; i < m_droneCount; ++i) {
        if (m_removed[i]) {
            continue;
        }

        if (m_flyOutDrone[i] && m_x[i] >= flyOutEndX - 1.0) {
            if (m_removeCallback) {
                m_removeCallback(i + 1);
            }
            m_flyOutDrone[i] = false;
            m_removed[i] = true;
            continue;
        }

        DroneState state;
        state.drone_id = i + 1;
        state.x = m_x[i];
        state.y = m_y[i];
        state.transitioning = m_flyInDrone[i] || m_flyOutDrone[i];

        if (m_flyInDrone[i]) {
            state.status = DroneStatus::ST_CRUISE;
        } else if (m_tickCount < 10) {
            state.status = DroneStatus::ST_IDLE;
        } else {
            state.status = DroneStatus::ST_CRUISE;
        }

        state.current_cmd_id = m_droneCmdIds[i];

        m_callback(state);
    }

    if (updateCmdId && m_latestCmdId > 0) {
        uint32_t idx = droneDist(m_rng);
        if (m_droneCmdIds[idx] < m_latestCmdId) {
            m_droneCmdIds[idx] = m_latestCmdId;
        }
    }
}