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
#include <QElapsedTimer>
#include <unordered_map>
#include <cstdint>

class ICommandSender;
class IStateReceiver;
class IDroneStateCache;
class IFormationChecker;
class DroneItem;
class CoordinateMapper;
class TestStateReceiver;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(ICommandSender* cmdSender,
                        IStateReceiver* stateReceiver,
                        IDroneStateCache* stateCache,
                        IFormationChecker* formationChecker,
                        QWidget* parent = nullptr);
    ~MainWindow() override = default;

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

private:
    void setupUi();
    void setupParameterPanel(QWidget* panel);
    void setupCanvas();
    void setupButtons(QWidget* container);
    void setupStatusBar();
    void updateDroneDisplay();

    ICommandSender*    m_cmdSender;
    IStateReceiver*    m_stateReceiver;
    IStateReceiver*    m_fakeReceiver = nullptr;   // 原始 Fake 接收器
    TestStateReceiver* m_testReceiver = nullptr;   // 测试模式接收器
    IDroneStateCache*  m_stateCache;
    IFormationChecker* m_formationChecker;

    QSpinBox*       m_spinN;
    QComboBox*      m_comboFormation;
    QLineEdit*      m_editFormationText;
    QDoubleSpinBox* m_spinCenterX;
    QDoubleSpinBox* m_spinCenterY;
    QDoubleSpinBox* m_spinSpacing;

    QGraphicsView*  m_graphicsView;
    QGraphicsScene* m_scene;

    QPushButton* m_btnTakeoff;
    QPushButton* m_btnFormation;
    QPushButton* m_btnLand;
    QPushButton* m_btnEmergency;

    QCheckBox* m_chkTestMode;

    CoordinateMapper* m_coordMapper;

    QTimer* m_completionTimer;
    QTimer* m_renderTimer;

    std::unordered_map<uint32_t, DroneItem*> m_droneItems;
    bool m_formationCompleted = false;
    QElapsedTimer m_renderMetricsTimer;
    uint32_t m_renderFramesInWindow = 0;
};
