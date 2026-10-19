#pragma once
#include <QMainWindow>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTimer>
#include <QStatusBar>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QEvent>
#include <QWheelEvent>
#include "core/DroneState.h"
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

class ICommandSender;
class IStateReceiver;
class IDroneStateCache;
class IFormationChecker;
class DroneItem;
class CoordinateMapper;
class GridScene;
class TestStateReceiver;
class StressTestReceiver;
class TelemetrySparkline;
struct StressPreset;
struct DroneCmd;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(ICommandSender* cmdSender,
                        IStateReceiver* stateReceiver,
                        IDroneStateCache* stateCache,
                        IFormationChecker* formationChecker,
                        QWidget* parent = nullptr);
    ~MainWindow() override;

    // Called in the UI thread when a fresh state sample reaches the controller.
    // This keeps arrival timestamps tied to receipt time, rather than to UI refresh.
    void observeStateForQuality(const DroneState& state);

    // Called after the DDS adapter receives a failover report from a surviving drone.
    // The quality panel must then follow C's new alive-ID-to-slot mapping.
    void handleDroneLost(uint32_t droneId);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onTakeoff();
    void onFormation();
    void onLand();
    void onEmergency();
    void onCompletionCheck();
    void onRenderTick();
    void onTestModeToggled(bool checked);
    void onStressPresetChanged(int index);

private:
    void setupUi();
    void setupParameterPanel(QWidget* panel);
    void setupCanvas();
    void setupButtons(QWidget* container);
    void setupStressTestPanel(QWidget* container);
    void setupTelemetryPanel(QWidget* panel);
    void setupQualityPanel(QWidget* panel);
    void setupStatusBar();
    void updateDroneDisplay();
    void updateStressStats();
    void updateCommunicationTelemetry();
    void updateQualityMetrics();
    void detectTimedOutDrones();
    bool beginQualityTracking(const DroneCmd& cmd);
    bool rebuildQualityTargets();

    ICommandSender*    m_cmdSender;
    IStateReceiver*    m_stateReceiver;
    IStateReceiver*    m_fakeReceiver = nullptr;
    TestStateReceiver* m_testReceiver = nullptr;
    StressTestReceiver* m_stressReceiver = nullptr;
    IDroneStateCache*  m_stateCache;
    IFormationChecker* m_formationChecker;

    QSpinBox*       m_spinN;
    QComboBox*      m_comboFormation;
    QLineEdit*      m_editFormationText;
    QDoubleSpinBox* m_spinCenterX;
    QDoubleSpinBox* m_spinCenterY;
    QDoubleSpinBox* m_spinSpacing;

    QGraphicsView*  m_graphicsView;
    GridScene*      m_scene;

    QPushButton* m_btnTakeoff;
    QPushButton* m_btnFormation;
    QPushButton* m_btnLand;
    QPushButton* m_btnEmergency;

    QCheckBox* m_chkTestMode;

    QComboBox* m_comboStressPreset;
    QLabel* m_lblStressStats;
    QLabel* m_lblMessageRate;
    QLabel* m_lblThroughput;
    TelemetrySparkline* m_telemetrySparkline;
    QLabel* m_lblMinDistance;
    QLabel* m_lblSyncError;
    QLabel* m_lblFormationResidual;

    CoordinateMapper* m_coordMapper;

    QTimer* m_completionTimer;
    QTimer* m_renderTimer;
    QTimer* m_stressStatsTimer;
    QTimer* m_telemetryTimer;

    std::unordered_map<uint32_t, DroneItem*> m_droneItems;
    struct TargetSlot {
        double x = 0.0;
        double y = 0.0;
    };
    struct QualityPlan {
        uint32_t formationId = 0;
        std::string formationText;
        uint32_t expectedCount = 0;
        double centerX = 0.0;
        double centerY = 0.0;
        double spacing = 0.0;
    };
    struct LastSeenState {
        std::chrono::steady_clock::time_point receivedAt;
        DroneStatus status = DroneStatus::ST_IDLE;
    };
    std::unordered_map<uint32_t, TargetSlot> m_qualityTargets;
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> m_arrivalTimes;
    std::unordered_map<uint32_t, LastSeenState> m_lastSeenStates;
    std::vector<uint32_t> m_qualityActiveDroneIds;
    std::unordered_set<uint32_t> m_lostDroneIds;
    QualityPlan m_qualityPlan;
    uint32_t m_qualityCmdId = 0;
    bool m_qualityTracking = false;
    bool m_formationCompleted = false;
};
