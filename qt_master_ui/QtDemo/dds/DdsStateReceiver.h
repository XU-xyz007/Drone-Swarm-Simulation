#pragma once

#include "core/IStateReceiver.h"
#include "../../../Communication/DdsService.h"

#include <QTimer>
#include <atomic>
#include <functional>

// 将 DDS 回调投递到 Qt 主线程，避免 DDS 线程直接操作 UI 状态。
class DdsStateReceiver : public QObject, public IStateReceiver {
    Q_OBJECT

public:
    explicit DdsStateReceiver(QObject* parent = nullptr);
    ~DdsStateReceiver() override;

    void setOnStateReceived(std::function<void(const DroneState&)> callback) override;
    void setOnDroneRemoved(std::function<void(uint32_t)> callback) override;
    void start() override;
    void stop() override;
    void setDroneCount(uint32_t count) override;

private:
    void onDdsState(const SharedMessage::DroneState& state);

    std::function<void(const DroneState&)> m_callback;
    std::function<void(uint32_t)> m_removeCallback;
    QTimer m_statsTimer;
    std::atomic<bool> m_running{false};
    std::atomic<uint64_t> m_samplesThisSecond{0};
    uint32_t m_expectedDroneCount = 0;
};
