#include "DdsStateReceiver.h"

#include "../../../Communication/DdsService.h"

#include <QMetaObject>
#include <iostream>

DdsStateReceiver::DdsStateReceiver(QObject* parent)
    : QObject(parent)
{
    m_statsTimer.setInterval(1000);
    connect(&m_statsTimer, &QTimer::timeout, this, [this]() {
        std::cout << "[DDS-UI] received "
                  << m_samplesThisSecond.exchange(0)
                  << " state samples/s | expected drones="
                  << m_expectedDroneCount << std::endl;
    });
}

DdsStateReceiver::~DdsStateReceiver()
{
    stop();
}

void DdsStateReceiver::setOnStateReceived(std::function<void(const DroneState&)> callback)
{
    m_callback = std::move(callback);
}

void DdsStateReceiver::setOnDroneRemoved(std::function<void(uint32_t)> callback)
{
    m_removeCallback = std::move(callback);
}

void DdsStateReceiver::setDroneCount(uint32_t count)
{
    m_expectedDroneCount = count;
}

void DdsStateReceiver::start()
{
    if (m_running.exchange(true)) return;

    SwarmComm::SubscribeState([this](const SharedMessage::DroneState& state) {
        onDdsState(state);
    });
    if (!SwarmComm::InitDDS()) {
        m_running = false;
        std::cerr << "[DDS-UI] InitDDS failed. Check ZRDDS_HOME, QoS XML and license." << std::endl;
        return;
    }
    m_statsTimer.start();
    std::cout << "[DDS-UI] state subscriber started on "
              << SwarmComm::kStateTopic << std::endl;
}

void DdsStateReceiver::stop()
{
    if (!m_running.exchange(false)) return;
    m_statsTimer.stop();
    SwarmComm::SubscribeState(SwarmComm::StateCallback());
    SwarmComm::ShutdownDDS();
}

void DdsStateReceiver::onDdsState(const SharedMessage::DroneState& state)
{
    if (!m_running) return;
    m_samplesThisSecond.fetch_add(1);

    DroneState uiState;
    uiState.drone_id = static_cast<uint32_t>(state.drone_id);
    uiState.status = static_cast<DroneStatus>(state.status);
    uiState.x = state.x;
    uiState.y = state.y;
    uiState.current_cmd_id = static_cast<uint32_t>(state.current_cmd_id);
    uiState.transitioning = (uiState.status == DroneStatus::ST_TAKEOFF ||
                             uiState.status == DroneStatus::ST_LANDING);

    QMetaObject::invokeMethod(this, [this, uiState]() {
        if (!m_running) return;
        if (m_callback) m_callback(uiState);
    }, Qt::QueuedConnection);
}
