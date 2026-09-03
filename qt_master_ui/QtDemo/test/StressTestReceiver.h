#pragma once
#include "core/IStateReceiver.h"
#include "TestPresets.h"
#include <QTimer>
#include <QObject>
#include <QElapsedTimer>
#include <functional>
#include <vector>
#include <random>
#include <cstdint>
#include <string>

class StressTestReceiver : public QObject, public IStateReceiver {
    Q_OBJECT

public:
    explicit StressTestReceiver(QObject* parent = nullptr);

    void setOnStateReceived(std::function<void(const DroneState&)> callback) override;
    void setOnDroneRemoved(std::function<void(uint32_t)> callback) override;
    void start() override;
    void stop() override;

    void setDroneCount(uint32_t count) override;
    void setLatestCmdId(uint32_t cmdId) override;

    void setPreset(const StressPreset& preset);
    const StressPreset* currentPreset() const { return m_hasPreset ? &m_currentPreset : nullptr; }

    double currentFps() const { return m_fps; }
    uint32_t messageCount() const { return m_totalMessages; }
    void resetStats();

private slots:
    void onTimerTick();

private:
    void applySeparation();
    void generateFormationTargets(uint32_t formationId);
    void updateFps();

    QTimer* m_timer = nullptr;
    std::function<void(const DroneState&)> m_callback;
    std::function<void(uint32_t)> m_removeCallback;

    StressPreset m_currentPreset;
    bool m_hasPreset = false;

    uint32_t m_droneCount = 0;
    uint32_t m_latestCmdId = 0;
    int m_tickCount = 0;
    double m_speed = 3.0;
    MovePattern m_pattern = MovePattern::RANDOM_WALK;
    bool m_autoFormation = false;
    int m_formationInterval = 30;
    uint32_t m_currentFormationId = 0;

    std::vector<double> m_x;
    std::vector<double> m_y;
    std::vector<double> m_phase;
    std::vector<double> m_targetX;
    std::vector<double> m_targetY;
    std::vector<uint32_t> m_droneCmdIds;
    std::mt19937 m_rng;

    double m_fps = -1.0;
    uint32_t m_totalMessages = 0;
    int m_fpsFrameCount = 0;
    int m_intervalMs = 100;
    QElapsedTimer m_fpsTimer;

    std::vector<bool> m_faultDrone;
    std::vector<bool> m_landingDrone;
    std::vector<bool> m_landedDrone;

    static constexpr double MIN_SEPARATION = 12.0;
    static constexpr double SEPARATION_STRENGTH = 8.0;
    static constexpr int SEPARATION_ITERATIONS = 3;
    static constexpr double SCENE_LIMIT = 150.0;
};