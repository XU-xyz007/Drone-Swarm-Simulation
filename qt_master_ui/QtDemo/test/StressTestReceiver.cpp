#include "StressTestReceiver.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <QElapsedTimer>

StressTestReceiver::StressTestReceiver(QObject* parent)
    : QObject(parent)
    , m_rng(std::random_device{}())
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &StressTestReceiver::onTimerTick);
}

void StressTestReceiver::setOnStateReceived(std::function<void(const DroneState&)> callback) {
    m_callback = callback;
}

void StressTestReceiver::setOnDroneRemoved(std::function<void(uint32_t)> callback) {
    m_removeCallback = callback;
}

void StressTestReceiver::setPreset(const StressPreset& preset) {
    m_timer->stop();

    m_currentPreset = preset;
    m_hasPreset = true;
    m_droneCount = preset.droneCount;
    m_speed = preset.speed;
    m_pattern = preset.pattern;
    m_autoFormation = preset.autoFormation;
    m_formationInterval = preset.formationIntervalTicks;
    m_currentFormationId = 0;

    int intervalMs = (preset.updateHz > 0) ? (1000 / preset.updateHz) : 100;
    m_intervalMs = intervalMs;
    m_timer->setInterval(intervalMs);
    std::cout << "[StressTest] setPreset: updateHz=" << preset.updateHz
              << " intervalMs=" << intervalMs << std::endl;

    m_x.assign(m_droneCount, 0.0);
    m_y.assign(m_droneCount, 0.0);
    m_phase.assign(m_droneCount, 0.0);
    m_targetX.assign(m_droneCount, 0.0);
    m_targetY.assign(m_droneCount, 0.0);
    m_droneCmdIds.assign(m_droneCount, 0);
    m_faultDrone.assign(m_droneCount, false);
    m_landingDrone.assign(m_droneCount, false);
    m_landedDrone.assign(m_droneCount, false);

    std::uniform_real_distribution<double> distX(-100.0, 100.0);
    std::uniform_real_distribution<double> distY(70.0, 130.0);
    std::uniform_real_distribution<double> distPhase(0.0, 2.0 * 3.141592653589793);

    for (uint32_t i = 0; i < m_droneCount; ++i) {
        m_x[i] = distX(m_rng);
        m_y[i] = distY(m_rng);
        m_phase[i] = distPhase(m_rng);
    }

    if (m_pattern == MovePattern::FORMATION_CYCLE) {
        generateFormationTargets(0);
    }

    m_tickCount = 0;
    m_totalMessages = 0;

    resetStats();

    std::cout << "[StressTest] 预设加载: " << preset.name
              << " | 无人机: " << preset.droneCount
              << " | 频率: " << preset.updateHz << "Hz"
              << " | 速度: " << preset.speed
              << " | 模式: " << static_cast<uint32_t>(preset.pattern)
              << " | 自动队形: " << (preset.autoFormation ? "是" : "否")
              << std::endl;
}

void StressTestReceiver::start() {
    if (m_droneCount == 0) return;
    resetStats();
    m_timer->start(m_intervalMs);
    std::cout << "[StressTest] 开始运行, interval=" << m_intervalMs << "ms" << std::endl;
}

void StressTestReceiver::stop() {
    m_timer->stop();
    std::cout << "[StressTest] 停止运行 | 总消息: " << m_totalMessages
              << " | 平均FPS: " << m_fps << std::endl;
}

void StressTestReceiver::setDroneCount(uint32_t count) {
    if (count == m_droneCount) return;
    uint32_t oldCount = m_droneCount;
    m_droneCount = count;
    m_x.resize(count, 0.0);
    m_y.resize(count, 0.0);
    m_phase.resize(count, 0.0);
    m_targetX.resize(count, 0.0);
    m_targetY.resize(count, 0.0);
    m_droneCmdIds.resize(count, 0);
    m_faultDrone.resize(count, false);
    m_landingDrone.resize(count, false);
    m_landedDrone.resize(count, false);

    if (count > oldCount) {
        std::uniform_real_distribution<double> distX(-100.0, 100.0);
        std::uniform_real_distribution<double> distY(70.0, 130.0);
        std::uniform_real_distribution<double> distPhase(0.0, 2.0 * 3.141592653589793);
        for (uint32_t i = oldCount; i < count; ++i) {
            m_x[i] = distX(m_rng);
            m_y[i] = distY(m_rng);
            m_phase[i] = distPhase(m_rng);
        }
    }
    if (m_removeCallback && count < oldCount) {
        for (uint32_t i = count; i < oldCount; ++i) {
            m_removeCallback(i + 1);
        }
    }
    std::cout << "[StressTest] 数量变更: " << oldCount << " -> " << count << std::endl;
}

void StressTestReceiver::setLatestCmdId(uint32_t cmdId) {
    m_latestCmdId = cmdId;
}

void StressTestReceiver::resetStats() {
    m_totalMessages = 0;
    m_fpsFrameCount = 0;
    m_fps = -1.0;
    m_fpsTimer.start();
}

void StressTestReceiver::generateFormationTargets(uint32_t formationId) {
    if (m_droneCount == 0) return;
    double cx = 0.0, cy = 100.0, spacing = 25.0;
    uint32_t count = m_droneCount;

    switch (formationId) {
    case 0: {
        int cols = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(count))));
        int rows = static_cast<int>(std::ceil(static_cast<double>(count) / cols));
        for (uint32_t i = 0; i < count; ++i) {
            int col = static_cast<int>(i) % cols;
            int row = static_cast<int>(i) / cols;
            double offsetX = (cols - 1) * spacing / 2.0;
            double offsetY = (rows - 1) * spacing / 2.0;
            m_targetX[i] = cx + col * spacing - offsetX;
            m_targetY[i] = cy + row * spacing - offsetY;
        }
        break;
    }
    case 1: {
        double offset = (count - 1) * spacing / 2.0;
        for (uint32_t i = 0; i < count; ++i) {
            m_targetX[i] = cx + i * spacing - offset;
            m_targetY[i] = cy;
        }
        break;
    }
    case 2: {
        int row = 0, col = 0;
        for (uint32_t i = 0; i < count; ++i) {
            double offsetX = row * spacing / 2.0;
            m_targetX[i] = cx + col * spacing - offsetX;
            m_targetY[i] = cy + row * spacing * 0.866;
            ++col;
            if (col > row) { col = 0; ++row; }
        }
        break;
    }
    case 3: {
        double radius = spacing * count / (2.0 * 3.141592653589793);
        if (radius < spacing) radius = spacing;
        for (uint32_t i = 0; i < count; ++i) {
            double angle = 2.0 * 3.141592653589793 * i / count;
            m_targetX[i] = cx + radius * std::cos(angle);
            m_targetY[i] = cy + radius * std::sin(angle);
        }
        break;
    }
    default:
        generateFormationTargets(0);
        return;
    }
}

void StressTestReceiver::applySeparation() {
    const double iterStrength = SEPARATION_STRENGTH / SEPARATION_ITERATIONS;
    for (int iter = 0; iter < SEPARATION_ITERATIONS; ++iter) {
        for (uint32_t i = 0; i < m_droneCount; ++i) {
            if (m_faultDrone[i] || m_landingDrone[i] || m_landedDrone[i]) continue;
            double sepX = 0.0, sepY = 0.0;
            for (uint32_t j = 0; j < m_droneCount; ++j) {
                if (i == j || m_faultDrone[j] || m_landingDrone[j] || m_landedDrone[j]) continue;
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

void StressTestReceiver::updateFps() {
    ++m_fpsFrameCount;
    qint64 elapsed = m_fpsTimer.elapsed();
    if (elapsed >= 1000) {
        m_fps = static_cast<double>(m_fpsFrameCount) * 1000.0 / static_cast<double>(elapsed);
        std::cout << "[StressTest] FPS更新: " << m_fps
                  << " (帧数=" << m_fpsFrameCount
                  << " 耗时=" << elapsed << "ms)" << std::endl;
        m_fpsFrameCount = 0;
        m_fpsTimer.restart();
    }
}

void StressTestReceiver::onTimerTick() {
    if (!m_callback || m_droneCount == 0) return;

    ++m_tickCount;
    updateFps();

    double dt = 1.0;

    if (m_autoFormation && m_tickCount % m_formationInterval == 0) {
        m_currentFormationId = (m_currentFormationId + 1) % 4;
        generateFormationTargets(m_currentFormationId);
        m_latestCmdId = m_tickCount;
        for (uint32_t i = 0; i < m_droneCount; ++i) {
            m_droneCmdIds[i] = m_latestCmdId;
        }
        std::cout << "[StressTest] 自动切换队形 -> " << m_currentFormationId
                  << " (tick=" << m_tickCount << ")" << std::endl;
    }

    if (m_tickCount == 60 && m_droneCount > 10) {
        uint32_t faultCount = m_droneCount / 10;
        for (uint32_t i = 0; i < faultCount; ++i) {
            m_faultDrone[i * 10 + 3] = true;
        }
        std::cout << "[StressTest] 注入 " << faultCount << " 架急停无人机" << std::endl;
    }
    if (m_tickCount == 120 && m_droneCount > 10) {
        uint32_t landCount = m_droneCount / 10;
        for (uint32_t i = 0; i < landCount; ++i) {
            m_landingDrone[i * 10 + 7] = true;
        }
        std::cout << "[StressTest] 注入 " << landCount << " 架降落无人机" << std::endl;
    }

    std::uniform_real_distribution<double> stepDist(-m_speed * 0.3, m_speed * 0.3);
    std::uniform_real_distribution<double> angleDist(0.0, 2.0 * 3.141592653589793);

    for (uint32_t i = 0; i < m_droneCount; ++i) {
        if (m_faultDrone[i] || m_landedDrone[i]) continue;

        if (m_landingDrone[i]) {
            m_y[i] -= m_speed * 0.5;
            if (m_y[i] < -SCENE_LIMIT) {
                m_landingDrone[i] = false;
                m_landedDrone[i] = true;
            }
            continue;
        }

        switch (m_pattern) {
        case MovePattern::RANDOM_WALK:
            m_x[i] += stepDist(m_rng);
            m_y[i] += stepDist(m_rng);
            break;

        case MovePattern::CIRCULAR: {
            double radius = 60.0 + (i % 5) * 15.0;
            double angularSpeed = m_speed * 0.02;
            m_phase[i] += angularSpeed;
            if (m_phase[i] > 2.0 * 3.141592653589793) m_phase[i] -= 2.0 * 3.141592653589793;
            m_x[i] = std::cos(m_phase[i]) * radius;
            m_y[i] = std::sin(m_phase[i]) * radius;
            m_x[i] += stepDist(m_rng) * 0.2;
            m_y[i] += stepDist(m_rng) * 0.2;
            break;
        }

        case MovePattern::WAVE: {
            double frequency = 0.02 + (i % 3) * 0.01;
            m_phase[i] += m_speed * 0.03;
            m_x[i] += m_speed * 0.3;
            m_y[i] = std::sin(m_phase[i] * frequency) * 60.0;
            if (m_x[i] > SCENE_LIMIT) m_x[i] = -SCENE_LIMIT;
            if (m_x[i] < -SCENE_LIMIT) m_x[i] = SCENE_LIMIT;
            break;
        }

        case MovePattern::ZIGZAG: {
            double zigzagSpeed = m_speed * 0.5;
            m_x[i] += zigzagSpeed;
            m_y[i] += (m_tickCount % 20 < 10) ? zigzagSpeed : -zigzagSpeed;
            if (m_x[i] > SCENE_LIMIT) { m_x[i] = -SCENE_LIMIT; m_y[i] = -m_y[i]; }
            if (m_x[i] < -SCENE_LIMIT) m_x[i] = SCENE_LIMIT;
            break;
        }

        case MovePattern::FORMATION_CYCLE: {
            double dx = m_targetX[i] - m_x[i];
            double dy = m_targetY[i] - m_y[i];
            double dist = std::sqrt(dx * dx + dy * dy);
            if (dist < 1.0) {
                m_x[i] = m_targetX[i];
                m_y[i] = m_targetY[i];
            } else {
                double step = std::min(m_speed, dist);
                m_x[i] += dx / dist * step;
                m_y[i] += dy / dist * step;
            }
            break;
        }
        }

        m_x[i] = std::clamp(m_x[i], -SCENE_LIMIT * 2, SCENE_LIMIT * 2);
        m_y[i] = std::clamp(m_y[i], -SCENE_LIMIT * 2, SCENE_LIMIT * 2);
    }

    applySeparation();

    for (uint32_t i = 0; i < m_droneCount; ++i) {
        DroneState state;
        state.drone_id = i + 1;
        state.x = m_x[i];
        state.y = m_y[i];
        state.transitioning = false;

        if (m_faultDrone[i]) {
            state.status = DroneStatus::ST_FAULT;
        } else if (m_landingDrone[i]) {
            state.status = DroneStatus::ST_LANDING;
        } else if (m_landedDrone[i]) {
            state.status = DroneStatus::ST_LANDED;
        } else if (m_pattern == MovePattern::FORMATION_CYCLE) {
            double dx = m_targetX[i] - m_x[i];
            double dy = m_targetY[i] - m_y[i];
            double dist = std::sqrt(dx * dx + dy * dy);
            state.status = (dist < 1.0) ? DroneStatus::ST_IDLE : DroneStatus::ST_CRUISE;
        } else {
            state.status = DroneStatus::ST_CRUISE;
        }
        state.current_cmd_id = m_droneCmdIds[i];

        m_callback(state);
        ++m_totalMessages;
    }
}