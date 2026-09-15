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
#include <QEvent>
#include <QWheelEvent>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <cstdint>

class ICommandSender;
class IStateReceiver;
class IDroneStateCache;
class IFormationChecker;
class DroneItem;
class CoordinateMapper;
class GridScene;
class TelemetrySparkline;
struct DroneState;
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

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onTakeoff();
    void onFormation();
    void onLand();
    void onEmergency();
    void onCompletionCheck();
    void onRenderTick();

private:
    void setupUi();
    void setupParameterPanel(QWidget* panel);
    void setupCanvas();
    void setupButtons(QWidget* container);
    void setupTelemetryPanel(QWidget* panel);
    void setupQualityPanel(QWidget* panel);
    void setupStatusBar();
    void updateDroneDisplay();
    void updateCommunicationTelemetry();
    void updateQualityMetrics();
    bool beginQualityTracking(const DroneCmd& cmd);

    ICommandSender*    m_cmdSender;
    IStateReceiver*    m_stateReceiver;
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

    QLabel* m_lblMessageRate;
    QLabel* m_lblThroughput;
    TelemetrySparkline* m_telemetrySparkline;
    QLabel* m_lblMinDistance;
    QLabel* m_lblSyncError;
    QLabel* m_lblFormationResidual;

    CoordinateMapper* m_coordMapper;

    QTimer* m_completionTimer;
    QTimer* m_renderTimer;
    QTimer* m_telemetryTimer;

    std::unordered_map<uint32_t, DroneItem*> m_droneItems;
    struct TargetSlot {
        double x = 0.0;
        double y = 0.0;
    };
    std::unordered_map<uint32_t, TargetSlot> m_qualityTargets;
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> m_arrivalTimes;
    uint32_t m_qualityCmdId = 0;
    bool m_qualityTracking = false;
    bool m_formationCompleted = false;
};