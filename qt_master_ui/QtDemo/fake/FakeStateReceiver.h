#pragma once
#include "core/IStateReceiver.h"
#include <QTimer>
#include <QObject>
#include <functional>
#include <vector>
#include <random>
#include <cstdint>

class FakeStateReceiver : public QObject, public IStateReceiver {
    Q_OBJECT

public:
    explicit FakeStateReceiver(QObject* parent = nullptr);

    void setOnStateReceived(std::function<void(const DroneState&)> callback) override;
    void setOnDroneRemoved(std::function<void(uint32_t)> callback) override;
    void start() override;
    void stop() override;

    void setDroneCount(uint32_t count);
    void setLatestCmdId(uint32_t cmdId);

private slots:
    void onTimerTick();

private:
    void applySeparation();

    QTimer* m_timer = nullptr;
    std::function<void(const DroneState&)> m_callback;
    uint32_t m_droneCount = 0;
    uint32_t m_userCount = 0;
    uint32_t m_latestCmdId = 0;

    std::vector<double> m_x;
    std::vector<double> m_y;
    std::vector<uint32_t> m_droneCmdIds;
    std::mt19937 m_rng;
    int m_tickCount = 0;

    std::vector<bool> m_flyOutDrone;
    std::vector<bool> m_flyInDrone;
    std::vector<bool> m_removed;
    std::function<void(uint32_t)> m_removeCallback;

    static constexpr double FLY_IN_OFFSET = 100.0;
    static constexpr double FLY_OUT_OFFSET = 100.0;
    static constexpr double MIN_SEPARATION = 15.0;
    static constexpr double SEPARATION_STRENGTH = 10.0;
    static constexpr int SEPARATION_ITERATIONS = 3;
};