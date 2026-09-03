#pragma once
#include "core/IStateReceiver.h"
#include <QTimer>
#include <QObject>
#include <functional>
#include <vector>
#include <cstdint>
#include <cmath>

class TestStateReceiver : public QObject, public IStateReceiver {
    Q_OBJECT

public:
    explicit TestStateReceiver(QObject* parent = nullptr);

    void setOnStateReceived(std::function<void(const DroneState&)> callback) override;
    void setOnDroneRemoved(std::function<void(uint32_t)> callback) override;
    void start() override;
    void stop() override;

    void setDroneCount(uint32_t count);
    void setLatestCmdId(uint32_t cmdId);
    void setFormationTarget(uint32_t formationId, double cx, double cy,
                            double spacing, uint32_t count);

private slots:
    void onTimerTick();

private:
    void generateTargets(uint32_t formationId, double cx, double cy,
                         double spacing, uint32_t count);

    QTimer* m_timer = nullptr;
    std::function<void(const DroneState&)> m_callback;
    uint32_t m_droneCount = 0;
    uint32_t m_userCount = 0;
    uint32_t m_latestCmdId = 0;

    void applySeparation(double strength);

    std::vector<double> m_x;
    std::vector<double> m_y;
    std::vector<double> m_targetX;
    std::vector<double> m_targetY;
    std::vector<uint32_t> m_droneCmdIds;
    std::vector<bool> m_reached;
    int m_tickCount = 0;

    std::vector<bool> m_faultDrone;
    std::vector<bool> m_landingDrone;
    std::vector<bool> m_landedDrone;

    std::vector<bool> m_flyOutDrone;
    std::vector<bool> m_flyInDrone;
    std::vector<bool> m_removed;
    std::vector<int> m_flyOutStartTick;
    std::function<void(uint32_t)> m_removeCallback;

    double m_formationCenterX = 0.0;
    double m_formationCenterY = 0.0;

    static constexpr double FLY_IN_OFFSET = 100.0;
    static constexpr double FLY_OUT_OFFSET = 100.0;
    static constexpr int FLY_OUT_DURATION = 30;
    static constexpr double MIN_SEPARATION = 15.0;
    static constexpr double SEPARATION_STRENGTH = 10.0;
    static constexpr int SEPARATION_ITERATIONS = 3;
};