#include "TestStateReceiver.h"
#include <algorithm>
#include <iostream>
#include <random>

TestStateReceiver::TestStateReceiver(QObject* parent)
    : QObject(parent)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &TestStateReceiver::onTimerTick);
}

void TestStateReceiver::setOnStateReceived(std::function<void(const DroneState&)> callback) {
    m_callback = callback;
}

void TestStateReceiver::setOnDroneRemoved(std::function<void(uint32_t)> callback) {
    m_removeCallback = callback;
}

void TestStateReceiver::start() {
    if (m_droneCount == 0) return;
    m_timer->start(100);
}

void TestStateReceiver::stop() {
    m_timer->stop();
}

void TestStateReceiver::setDroneCount(uint32_t count) {
    if (count == m_userCount) return;

    double flyInStartX = m_formationCenterX - FLY_IN_OFFSET;

    if (count < m_userCount) {
        m_userCount = count;
        for (uint32_t i = count; i < m_droneCount; ++i) {
            m_flyOutDrone[i] = true;
            m_flyOutStartTick[i] = m_tickCount;
            std::cout << "[Test] 无人机 " << (i + 1) << " 开始向右飞出"
                      << " (userCount=" << m_userCount
                      << " droneCount=" << m_droneCount << ")" << std::endl;
        }
        return;
    }

    uint32_t oldUserCount = m_userCount;
    uint32_t oldDroneCount = m_droneCount;
    m_userCount = count;
    m_droneCount = count;
    m_x.resize(count);
    m_y.resize(count);
    m_targetX.resize(count, 0.0);
    m_targetY.resize(count, 0.0);
    m_droneCmdIds.resize(count, 0);
    m_reached.resize(count, false);
    m_faultDrone.resize(count, false);
    m_landingDrone.resize(count, false);
    m_landedDrone.resize(count, false);
    m_flyOutDrone.resize(count, false);
    m_flyInDrone.resize(count, false);
    m_removed.resize(count, false);
    m_flyOutStartTick.resize(count, 0);

    if (count > oldDroneCount) {
        std::mt19937 rng(42 + static_cast<unsigned>(oldDroneCount));
        std::uniform_real_distribution<double> distY(-80.0, 80.0);
        for (uint32_t i = oldDroneCount; i < count; ++i) {
            m_x[i] = flyInStartX;
            m_y[i] = distY(rng);
            m_flyInDrone[i] = true;
            m_reached[i] = false;
            std::cout << "[Test] 无人机 " << (i + 1) << " 从左侧飞入 ("
                      << m_x[i] << "," << m_y[i] << ")" << std::endl;
        }
    } else if (count > oldUserCount) {
        for (uint32_t i = oldUserCount; i < count; ++i) {
            m_flyOutDrone[i] = false;
            m_removed[i] = false;
            m_flyInDrone[i] = true;
            m_reached[i] = false;
            m_x[i] = flyInStartX;
            std::cout << "[Test] 无人机 " << (i + 1) << " 重新激活，从左侧飞入" << std::endl;
        }
    }
}

void TestStateReceiver::setLatestCmdId(uint32_t cmdId) {
    m_latestCmdId = cmdId;
}

void TestStateReceiver::setFormationTarget(uint32_t formationId, double cx, double cy,
                                           double spacing, uint32_t count) {
    m_formationCenterX = cx;
    m_formationCenterY = cy;
    uint32_t activeCount = m_userCount > 0 ? m_userCount : m_droneCount;
    if (count > activeCount) count = activeCount;
    generateTargets(formationId, cx, cy, spacing, count);
    for (uint32_t i = 0; i < m_droneCount; ++i) {
        if (!m_removed[i] && !m_flyOutDrone[i]) {
            m_reached[i] = false;
        }
    }
    std::cout << "[Test] 生成队形目标: formationId=" << formationId
              << " center=(" << cx << "," << cy << ")"
              << " spacing=" << spacing << " count=" << count
              << " activeCount=" << activeCount << std::endl;
}

void TestStateReceiver::generateTargets(uint32_t formationId, double cx, double cy,
                                        double spacing, uint32_t count) {
    if (count == 0) return;

    switch (formationId) {
    case 0: {
        // 方阵: 排列成 cols × rows 网格
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
        // 一字: 水平排列
        double offset = (count - 1) * spacing / 2.0;
        for (uint32_t i = 0; i < count; ++i) {
            m_targetX[i] = cx + i * spacing - offset;
            m_targetY[i] = cy;
        }
        break;
    }
    case 2: {
        // 三角: 等腰三角形排列
        int row = 0, col = 0, placed = 0;
        double offsetX = 0;
        for (uint32_t i = 0; i < count; ++i) {
            if (col == 0) {
                offsetX = row * spacing / 2.0;
            }
            m_targetX[i] = cx + col * spacing - offsetX;
            m_targetY[i] = cy + row * spacing * 0.866;
            ++col;
            ++placed;
            if (col > row) {
                col = 0;
                ++row;
            }
        }
        break;
    }
    case 3: {
        // 圆: 等角度分布在圆周上
        double radius = spacing * count / (2.0 * 3.141592653589793);
        if (radius < spacing) radius = spacing;
        for (uint32_t i = 0; i < count; ++i) {
            double angle = 2.0 * 3.141592653589793 * i / count;
            m_targetX[i] = cx + radius * std::cos(angle);
            m_targetY[i] = cy + radius * std::sin(angle);
        }
        break;
    }
    case 4: {
        // 字母 D (5列 × 7行)
        const int shapeCols = 5, shapeRows = 7;
        const int shapePoints[][2] = {
            {0,0},{1,0},{2,0},{3,0},
            {0,1},{4,1},
            {0,2},{4,2},
            {0,3},{4,3},
            {0,4},{4,4},
            {0,5},{4,5},
            {0,6},{1,6},{2,6},{3,6},
        };
        const int total = sizeof(shapePoints) / sizeof(shapePoints[0]);
        double offsetX = (shapeCols - 1) * spacing / 2.0;
        double offsetY = (shapeRows - 1) * spacing / 2.0;
        for (uint32_t i = 0; i < count; ++i) {
            int idx = i % total;
            m_targetX[i] = cx + shapePoints[idx][0] * spacing - offsetX;
            m_targetY[i] = cy + shapePoints[idx][1] * spacing - offsetY;
        }
        break;
    }
    case 5: {
        // 字母 S (5列 × 7行)
        const int shapeCols = 5, shapeRows = 7;
        const int shapePoints[][2] = {
            {1,0},{2,0},{3,0},{4,0},
            {0,1},
            {0,2},
            {1,3},{2,3},{3,3},
            {4,4},
            {4,5},
            {0,6},{1,6},{2,6},{3,6},
        };
        const int total = sizeof(shapePoints) / sizeof(shapePoints[0]);
        double offsetX = (shapeCols - 1) * spacing / 2.0;
        double offsetY = (shapeRows - 1) * spacing / 2.0;
        for (uint32_t i = 0; i < count; ++i) {
            int idx = i % total;
            m_targetX[i] = cx + shapePoints[idx][0] * spacing - offsetX;
            m_targetY[i] = cy + shapePoints[idx][1] * spacing - offsetY;
        }
        break;
    }
    default:
        // 默认方阵
        generateTargets(0, cx, cy, spacing, count);
        break;
    }
}

void TestStateReceiver::applySeparation(double strength) {
    const double iterStrength = strength / SEPARATION_ITERATIONS;
    for (int iter = 0; iter < SEPARATION_ITERATIONS; ++iter) {
        for (uint32_t i = 0; i < m_droneCount; ++i) {
            if (m_flyOutDrone[i] || m_removed[i]) continue;
            if (m_reached[i]) continue;
            double tdx = m_targetX[i] - m_x[i];
            double tdy = m_targetY[i] - m_y[i];
            double distToTarget = std::sqrt(tdx * tdx + tdy * tdy);
            if (distToTarget < 3.0) continue;
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

void TestStateReceiver::onTimerTick() {
    if (!m_callback || m_droneCount == 0) return;

    ++m_tickCount;
    const double speed = 3.0;
    const double flyOutEndX = m_formationCenterX + FLY_OUT_OFFSET;

    if (m_tickCount == 30 && m_droneCount > 1) {
        m_faultDrone[1] = true;
        std::cout << "[Test] 无人机 2 急停 (ST_FAULT)" << std::endl;
    }
    if (m_tickCount == 40 && m_droneCount > 4) {
        m_faultDrone[4] = true;
        std::cout << "[Test] 无人机 5 急停 (ST_FAULT)" << std::endl;
    }
    if (m_tickCount == 60 && m_droneCount > 2) {
        m_landingDrone[2] = true;
        std::cout << "[Test] 无人机 3 开始降落 (ST_LANDING)" << std::endl;
    }

    for (uint32_t i = 0; i < m_droneCount; ++i) {
        if (m_flyOutDrone[i]) {
            double dx = flyOutEndX - m_x[i];
            if (std::abs(dx) < 2.0) {
                m_x[i] = flyOutEndX;
                if (m_removeCallback) {
                    m_removeCallback(i + 1);
                }
                m_flyOutDrone[i] = false;
                m_removed[i] = true;
                std::cout << "[Test] 无人机 " << (i + 1) << " 已飞出画面" << std::endl;
            } else {
                m_x[i] += (dx > 0 ? 1.0 : -1.0) * speed;
            }
            continue;
        }

        if (m_removed[i]) {
            continue;
        }

        if (m_faultDrone[i]) {
            continue;
        }
        if (m_landingDrone[i]) {
            m_y[i] -= 2.0;
            if (m_y[i] < -120.0) {
                m_landingDrone[i] = false;
                m_landedDrone[i] = true;
                std::cout << "[Test] 无人机 " << (i + 1) << " 已降落 (ST_LANDED)" << std::endl;
            }
            continue;
        }
        if (m_landedDrone[i]) {
            continue;
        }
        if (!m_reached[i]) {
            double dx = m_targetX[i] - m_x[i];
            double dy = m_targetY[i] - m_y[i];
            double dist = std::sqrt(dx * dx + dy * dy);

            if (dist < 1.0) {
                m_x[i] = m_targetX[i];
                m_y[i] = m_targetY[i];
                m_reached[i] = true;
                m_flyInDrone[i] = false;
                if (m_latestCmdId > 0 && m_droneCmdIds[i] < m_latestCmdId) {
                    m_droneCmdIds[i] = m_latestCmdId;
                }
                std::cout << "[Test] 无人机 " << (i + 1) << " 到达目标 ("
                          << m_targetX[i] << "," << m_targetY[i] << ")" << std::endl;
            } else {
                double step = std::min(speed, dist);
                m_x[i] += dx / dist * step;
                m_y[i] += dy / dist * step;
            }
        }
    }

    applySeparation(SEPARATION_STRENGTH);

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
            std::cout << "[Test] 无人机 " << (i + 1) << " 已飞出画面" << std::endl;
            continue;
        }

        DroneState state;
        state.drone_id = i + 1;
        state.x = m_x[i];
        state.y = m_y[i];
        state.transitioning = m_flyInDrone[i] || m_flyOutDrone[i];

        if (m_faultDrone[i]) {
            state.status = DroneStatus::ST_FAULT;
        } else if (m_landingDrone[i]) {
            state.status = DroneStatus::ST_LANDING;
        } else if (m_landedDrone[i]) {
            state.status = DroneStatus::ST_LANDED;
        } else if (m_flyInDrone[i]) {
            state.status = DroneStatus::ST_CRUISE;
        } else {
            state.status = m_reached[i] ? DroneStatus::ST_IDLE : DroneStatus::ST_CRUISE;
        }
        state.current_cmd_id = m_droneCmdIds[i];

        m_callback(state);
    }
}