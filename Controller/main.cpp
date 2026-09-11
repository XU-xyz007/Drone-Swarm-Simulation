#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QMetaObject>
#include <QTimer>

#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <windows.h>

#include "../qt_master_ui/QtDemo/MainWindow.h"
#include "../qt_master_ui/QtDemo/core/DroneStateCache.h"
#include "../qt_master_ui/QtDemo/core/FormationChecker.h"
#include "../qt_master_ui/QtDemo/core/ICommandSender.h"
#include "../qt_master_ui/QtDemo/core/IStateReceiver.h"
#include "../Communication/DdsService.h"

namespace {

class DdsTelemetry {
public:
    void record(const SharedMessage::DroneState& state) {
        std::lock_guard<std::mutex> lock(m_mutex);
        ++m_samples;
        m_payloadBytes += SwarmComm::EstimateStatePayloadBytes(state);
        const std::map<DDS_ULong, DDS_ULong>::const_iterator last = m_lastSequenceByDrone.find(state.drone_id);
        if (last != m_lastSequenceByDrone.end()) {
            if (state.seq_num > last->second + 1) m_sequenceGaps += state.seq_num - last->second - 1;
            else if (state.seq_num <= last->second) ++m_outOfOrder;
        }
        m_lastSequenceByDrone[state.drone_id] = state.seq_num;
    }

    void reportAndReset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        const double kbitPerSecond = static_cast<double>(m_payloadBytes * 8ULL) / 1000.0;
        std::cout << "[DDS telemetry] " << m_samples << " state samples/s | "
                  << m_payloadBytes << " payload bytes/s (" << kbitPerSecond
                  << " kbit/s) | seq gaps: " << m_sequenceGaps
                  << " | out-of-order: " << m_outOfOrder << std::endl;
        m_samples = m_payloadBytes = m_sequenceGaps = m_outOfOrder = 0;
    }

private:
    std::mutex m_mutex;
    std::map<DDS_ULong, DDS_ULong> m_lastSequenceByDrone;
    unsigned long long m_samples = 0;
    unsigned long long m_payloadBytes = 0;
    unsigned long long m_sequenceGaps = 0;
    unsigned long long m_outOfOrder = 0;
};

} // namespace

class RealCommandSender : public ICommandSender {
public:
    bool sendCommand(const DroneCmd& cmd) override {
        m_lastCmdId = cmd.cmd_id;
        SharedMessage::DroneCmd ddsCmd;
        SharedMessage::DroneCmdInitialize(&ddsCmd);
        ddsCmd.cmd_id = cmd.cmd_id;
        ddsCmd.cmd_type = static_cast<SharedMessage::DroneCmdType>(cmd.cmd_type);
        ddsCmd.target_x = cmd.target_x;
        ddsCmd.target_y = cmd.target_y;
        ddsCmd.spacing = cmd.spacing;
        ddsCmd.formation_id = cmd.formation_id;
        ddsCmd.expected_count = cmd.expected_count;
        if (!cmd.formation_text.empty()) {
            strncpy(ddsCmd.formation_text, cmd.formation_text.c_str(), 255);
            ddsCmd.formation_text[255] = '\0';
        }
        const bool ok = SwarmComm::PublishCmd(ddsCmd);
        SharedMessage::DroneCmdFinalize(&ddsCmd);
        if (!ok) std::cerr << "[DDS Error] Command publish failed, cmd_id=" << cmd.cmd_id << std::endl;
        return ok;
    }

    uint32_t getLastCmdId() const override { return m_lastCmdId; }

private:
    uint32_t m_lastCmdId = 0;
};

class RealStateReceiver : public IStateReceiver {
public:
    void setOnStateReceived(std::function<void(const DroneState&)> callback) override { m_onStateReceived = callback; }
    void setOnDroneRemoved(std::function<void(uint32_t)> callback) override { m_onDroneRemoved = callback; }
    void setLatestCmdId(uint32_t cmdId) override { m_latestCmdId = cmdId; }

    void start() override {
        SwarmComm::SubscribeState([this](const SharedMessage::DroneState& ddsState) {
            m_telemetry.record(ddsState);
            QMetaObject::invokeMethod(qApp, [this, ddsState]() {
                if (!m_onStateReceived) return;
                DroneState state;
                state.drone_id = ddsState.drone_id;
                state.x = ddsState.x;
                state.y = ddsState.y;
                state.vx = ddsState.vx;
                state.vy = ddsState.vy;
                state.status = static_cast<DroneStatus>(ddsState.status);
                state.current_cmd_id = ddsState.current_cmd_id;
                state.seq_num = ddsState.seq_num;
                state.transitioning = false;
                m_onStateReceived(state);
            }, Qt::QueuedConnection);
        });
        std::cout << "[DDS] State subscription started." << std::endl;
    }

    void stop() override {}
    void reportTelemetry() { m_telemetry.reportAndReset(); }

private:
    std::function<void(const DroneState&)> m_onStateReceived;
    std::function<void(uint32_t)> m_onDroneRemoved;
    uint32_t m_latestCmdId = 0;
    DdsTelemetry m_telemetry;
};

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);
    QApplication application(argc, argv);
    if (!QFileInfo::exists("ZRDDS_QOS_PROFILES.xml")) {
        QMessageBox::critical(nullptr, "Startup failed", "Missing ZRDDS_QOS_PROFILES.xml next to the executable.");
        return -1;
    }
    if (!QFileInfo::exists("zrddslicence.lic")) {
        QMessageBox::critical(nullptr, "Startup failed", "Missing zrddslicence.lic next to the executable.");
        return -1;
    }
    if (!SwarmComm::InitDDS()) {
        QMessageBox::critical(nullptr, "Startup failed", "ZR-DDS initialization failed.");
        return -1;
    }

    RealCommandSender commandSender;
    RealStateReceiver stateReceiver;
    DroneStateCache stateCache;
    FormationChecker formationChecker;
    stateReceiver.setOnStateReceived([&](const DroneState& state) {
        stateCache.updateState(state);
        formationChecker.updateDroneCmdId(state.drone_id, state.current_cmd_id);
        formationChecker.updateDroneStatus(state.drone_id, state.status);
    });

    MainWindow window(&commandSender, &stateReceiver, &stateCache, &formationChecker);
    QTimer telemetryTimer;
    QObject::connect(&telemetryTimer, &QTimer::timeout, [&stateReceiver]() { stateReceiver.reportTelemetry(); });
    telemetryTimer.start(1000);
    stateReceiver.start();
    window.show();
    const int result = application.exec();
    SwarmComm::ShutdownDDS();
    return result;
}
