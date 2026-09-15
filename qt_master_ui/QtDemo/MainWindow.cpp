#include "MainWindow.h"
#include "core/ICommandSender.h"
#include "core/IStateReceiver.h"
#include "core/IDroneStateCache.h"
#include "core/IFormationChecker.h"
#include "core/DroneCmd.h"
#include "core/DroneState.h"
// Qt defines a `slots` macro for signal/slot declarations. B's public
// FormationResult has a member with the same name, so suspend the Qt macro
// only while parsing B's contract, then restore it for the remaining Qt headers.
#ifdef slots
#pragma push_macro("slots")
#undef slots
#define RESTORE_QT_SLOTS_MACRO
#endif
#include "FormationAlgorithm.h"
#ifdef RESTORE_QT_SLOTS_MACRO
#pragma pop_macro("slots")
#undef RESTORE_QT_SLOTS_MACRO
#endif
#include "render/DroneItem.h"
#include "render/ColorConfig.h"
#include "render/CoordinateMapper.h"
#include "render/GridScene.h"
#include "render/TelemetrySparkline.h"
#include "test/TestStateReceiver.h"
#include "test/StressTestReceiver.h"
#include "test/TestPresets.h"
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QLocale>
#include <QWheelEvent>
#include <QScrollBar>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <unordered_set>
#include <utility>

namespace {
// Keep the receipt-side arrival statistic consistent with C's current
// 0.10 m control deadband.  This is deliberately an internal threshold:
// the UI presents the resulting metric rather than a second control setting.
constexpr double kArrivalThresholdMeters = 0.10;

QString metricStyle(bool alert = false) {
    return alert
        ? "QLabel{background:#3b2427;color:#ffb4a8;border:1px solid #914447;border-radius:5px;padding:7px 8px;font-weight:600;font-size:17px;}"
        : "QLabel{background:#14242f;color:#d9f9f5;border:1px solid #294452;border-radius:5px;padding:7px 8px;font-weight:600;font-size:17px;}";
}
}

MainWindow::MainWindow(ICommandSender* cmdSender,
                       IStateReceiver* stateReceiver,
                       IDroneStateCache* stateCache,
                       IFormationChecker* formationChecker,
                       QWidget* parent)
    : QMainWindow(parent)
    , m_cmdSender(cmdSender)
    , m_stateReceiver(stateReceiver)
    , m_fakeReceiver(stateReceiver)
    , m_stateCache(stateCache)
    , m_formationChecker(formationChecker)
{
    setWindowTitle("无人机编队主控端");
    resize(1160, 760);

    m_testReceiver = new TestStateReceiver(this);
    m_testReceiver->setDroneCount(8);
    m_testReceiver->setOnStateReceived([this](const DroneState& state) {
        m_stateCache->updateState(state);
        m_formationChecker->updateDroneCmdId(state.drone_id, state.current_cmd_id);
        m_formationChecker->updateDroneStatus(state.drone_id, state.status);
        observeStateForQuality(state);
    });
    m_testReceiver->setOnDroneRemoved([this](uint32_t droneId) {
        m_stateCache->removeState(droneId);
    });

    m_stressReceiver = new StressTestReceiver(this);
    m_stressReceiver->setOnStateReceived([this](const DroneState& state) {
        m_stateCache->updateState(state);
        m_formationChecker->updateDroneCmdId(state.drone_id, state.current_cmd_id);
        m_formationChecker->updateDroneStatus(state.drone_id, state.status);
        observeStateForQuality(state);
    });
    m_stressReceiver->setOnDroneRemoved([this](uint32_t droneId) {
        m_stateCache->removeState(droneId);
    });

    m_coordMapper = new CoordinateMapper();

    setupUi();
    setupStatusBar();

    m_completionTimer = new QTimer(this);
    connect(m_completionTimer, &QTimer::timeout, this, &MainWindow::onCompletionCheck);
    m_completionTimer->start(200);

    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, &MainWindow::onRenderTick);
    m_renderTimer->start(50);

    m_stressStatsTimer = new QTimer(this);
    connect(m_stressStatsTimer, &QTimer::timeout, this, &MainWindow::updateStressStats);
    m_stressStatsTimer->start(1000);

    m_telemetryTimer = new QTimer(this);
    connect(m_telemetryTimer, &QTimer::timeout, this, &MainWindow::updateCommunicationTelemetry);
    m_telemetryTimer->start(1000);
}

MainWindow::~MainWindow() {
    m_droneItems.clear();
}

void MainWindow::setupUi() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    QWidget* leftPanel = new QWidget();
    leftPanel->setFixedWidth(240);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    setupParameterPanel(leftPanel);
    leftLayout->addWidget(leftPanel->findChild<QGroupBox*>("paramGroup"));

    setupButtons(leftPanel);
    setupStressTestPanel(leftPanel);
    if (QGroupBox* stressGroup = leftPanel->findChild<QGroupBox*>("stressGroup")) {
        stressGroup->hide();
    }
    leftLayout->addStretch();

    mainLayout->addWidget(leftPanel);

    setupCanvas();
    mainLayout->addWidget(m_graphicsView, 1);

    QWidget* rightPanel = new QWidget();
    rightPanel->setFixedWidth(330);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    setupTelemetryPanel(rightPanel);
    setupQualityPanel(rightPanel);
    rightLayout->addStretch();
    mainLayout->addWidget(rightPanel);
}

void MainWindow::setupParameterPanel(QWidget* panel) {
    QGroupBox* group = new QGroupBox("参数设置", panel);
    group->setObjectName("paramGroup");
    QFormLayout* form = new QFormLayout(group);

    m_spinN = new QSpinBox();
    m_spinN->setRange(1, 999);
    m_spinN->setValue(8);
    m_spinN->setToolTip("无人机数量 (1~999；较大规模会显著增加本机 CPU 与线程负载)");
    form->addRow("无人机数量 N:", m_spinN);

    connect(m_spinN, QOverload<int>::of(&QSpinBox::valueChanged),
        [this](int value) {
            std::cout << "[UI] 无人机数量变更为: " << value << std::endl;
            m_fakeReceiver->setDroneCount(static_cast<uint32_t>(value));
            m_testReceiver->setDroneCount(static_cast<uint32_t>(value));
        });

    m_comboFormation = new QComboBox();
    m_comboFormation->addItem("0 - 方阵");
    m_comboFormation->addItem("1 - 一字");
    m_comboFormation->addItem("2 - 三角");
    m_comboFormation->addItem("3 - 圆形");
    m_comboFormation->addItem("文字队形");
    m_comboFormation->setToolTip("选择队形类型");
    form->addRow("队形:", m_comboFormation);

    m_editFormationText = new QLineEdit();
    m_editFormationText->setMaxLength(16);
    m_editFormationText->setPlaceholderText("文字队形时填写，最多16字符");
    m_editFormationText->setEnabled(false);
    form->addRow("文字队形:", m_editFormationText);

    connect(m_comboFormation, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                m_editFormationText->setEnabled(index == 4);
            });

    m_spinCenterX = new QDoubleSpinBox();
    m_spinCenterX->setRange(-1000.0, 1000.0);
    m_spinCenterX->setValue(0.0);
    m_spinCenterX->setDecimals(1);
    m_spinCenterX->setSuffix(" m");
    form->addRow("中心 X:", m_spinCenterX);

    m_spinCenterY = new QDoubleSpinBox();
    m_spinCenterY->setRange(-1000.0, 1000.0);
    m_spinCenterY->setValue(100.0);
    m_spinCenterY->setDecimals(1);
    m_spinCenterY->setSuffix(" m");
    form->addRow("中心 Y:", m_spinCenterY);

    m_spinSpacing = new QDoubleSpinBox();
    m_spinSpacing->setRange(0.5, 100.0);
    m_spinSpacing->setValue(5.0);
    m_spinSpacing->setDecimals(1);
    m_spinSpacing->setSuffix(" m");
    form->addRow("间距:", m_spinSpacing);

    m_chkTestMode = new QCheckBox("测试模式（确定性队形）");
    m_chkTestMode->setToolTip("勾选后无人机按确定队形目标飞行，可验证队形完成判定");
    connect(m_chkTestMode, &QCheckBox::toggled, this, &MainWindow::onTestModeToggled);
    form->addRow(m_chkTestMode);
    m_chkTestMode->hide();
    form->setRowVisible(m_chkTestMode, false);

    QVBoxLayout* panelLayout = qobject_cast<QVBoxLayout*>(panel->layout());
    if (panelLayout) {
        panelLayout->insertWidget(0, group);
    }
}

void MainWindow::setupCanvas() {
    m_scene = new GridScene(this);
    m_scene->setSceneRect(-50, -50, 100, 100);

    m_graphicsView = new QGraphicsView(m_scene);
    m_graphicsView->setRenderHint(QPainter::Antialiasing);
    m_graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    m_graphicsView->setBackgroundBrush(QColor(30, 30, 30));
    m_graphicsView->setMinimumWidth(400);
    m_graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    m_graphicsView->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    m_graphicsView->viewport()->installEventFilter(this);
}

void MainWindow::setupButtons(QWidget* container) {
    QVBoxLayout* btnLayout = new QVBoxLayout();
    btnLayout->setSpacing(6);

    m_btnTakeoff = new QPushButton("起 飞");
    m_btnTakeoff->setMinimumHeight(36);
    m_btnTakeoff->setStyleSheet("QPushButton{background-color:#4CAF50;color:white;font-weight:bold;font-size:14px;border-radius:4px;}");

    m_btnFormation = new QPushButton("切换队形");
    m_btnFormation->setMinimumHeight(36);
    m_btnFormation->setStyleSheet("QPushButton{background-color:#2196F3;color:white;font-weight:bold;font-size:14px;border-radius:4px;}");

    m_btnLand = new QPushButton("降 落");
    m_btnLand->setMinimumHeight(36);
    m_btnLand->setStyleSheet("QPushButton{background-color:#FF9800;color:white;font-weight:bold;font-size:14px;border-radius:4px;}");

    m_btnEmergency = new QPushButton("急 停");
    m_btnEmergency->setMinimumHeight(36);
    m_btnEmergency->setStyleSheet("QPushButton{background-color:#F44336;color:white;font-weight:bold;font-size:14px;border-radius:4px;}");

    btnLayout->addWidget(m_btnTakeoff);
    btnLayout->addWidget(m_btnFormation);
    btnLayout->addWidget(m_btnLand);
    btnLayout->addWidget(m_btnEmergency);

    QVBoxLayout* containerLayout = qobject_cast<QVBoxLayout*>(container->layout());
    if (containerLayout) {
        containerLayout->addLayout(btnLayout);
    }

    connect(m_btnTakeoff,    &QPushButton::clicked, this, &MainWindow::onTakeoff);
    connect(m_btnFormation,  &QPushButton::clicked, this, &MainWindow::onFormation);
    connect(m_btnLand,       &QPushButton::clicked, this, &MainWindow::onLand);
    connect(m_btnEmergency,  &QPushButton::clicked, this, &MainWindow::onEmergency);
}

void MainWindow::setupStatusBar() {
    statusBar()->showMessage("就绪");
}

void MainWindow::onTakeoff() {
    int formationIdx = m_comboFormation->currentIndex();
    DroneCmd cmd = {};
    cmd.cmd_id = m_cmdSender->getLastCmdId() + 1;
    cmd.cmd_type = DroneCmdType::CMD_TAKEOFF;
    cmd.target_x = m_spinCenterX->value();
    cmd.target_y = m_spinCenterY->value();
    cmd.spacing = m_spinSpacing->value();
    cmd.expected_count = static_cast<uint32_t>(m_spinN->value());

    if (formationIdx == 4) {
        cmd.formation_id = 0;
        cmd.formation_text = m_editFormationText->text().trimmed().toUpper().toStdString();
    } else {
        cmd.formation_id = static_cast<uint32_t>(formationIdx);
    }

    // ========== 加在 sendCommand 之前 ==========
    printf("[DEBUG] MainWindow发送前 formation_text 是否为空: %d\n", cmd.formation_text.empty());
    printf("[DEBUG] MainWindow发送前 内容: [%s]\n", cmd.formation_text.c_str());
    // ==========================================

    if (!m_cmdSender->sendCommand(cmd)) {
        statusBar()->showMessage("起飞命令发送失败");
        return;
    }
    beginQualityTracking(cmd);



    m_formationChecker->setLatestCmdId(cmd.cmd_id);
    m_stateReceiver->setLatestCmdId(cmd.cmd_id);
    m_testReceiver->setLatestCmdId(cmd.cmd_id);

    statusBar()->showMessage("已发送起飞命令");
    std::cout << "========== 起飞 ==========" << std::endl;
}

void MainWindow::onFormation() {
    int formationIdx = m_comboFormation->currentIndex();
    DroneCmd cmd = {};
    cmd.cmd_id = m_cmdSender->getLastCmdId() + 1;
    cmd.cmd_type = DroneCmdType::CMD_FORMATION;
    cmd.target_x = m_spinCenterX->value();
    cmd.target_y = m_spinCenterY->value();
    cmd.spacing = m_spinSpacing->value();
    cmd.expected_count = static_cast<uint32_t>(m_spinN->value());

    if (formationIdx == 4) {
        cmd.formation_id = 0;
        cmd.formation_text = m_editFormationText->text().trimmed().toUpper().toStdString();
    } else {
        cmd.formation_id = static_cast<uint32_t>(formationIdx);
        cmd.formation_text = "";
    }

    // ========== 加在 sendCommand 之前 ==========
    printf("[DEBUG] MainWindow发送前 formation_text 是否为空: %d\n", cmd.formation_text.empty());
    printf("[DEBUG] MainWindow发送前 内容: [%s]\n", cmd.formation_text.c_str());
    // ==========================================

    if (!m_cmdSender->sendCommand(cmd)) {
        statusBar()->showMessage("队形命令发送失败");
        return;
    }
    if (!beginQualityTracking(cmd)) {
        statusBar()->showMessage("队形命令已发送，但目标槽位计算失败");
    }



    uint32_t N = static_cast<uint32_t>(m_spinN->value());
    m_formationChecker->setExpectedCount(N);
    m_formationChecker->setLatestCmdId(cmd.cmd_id);
    m_stateReceiver->setLatestCmdId(cmd.cmd_id);

    m_testReceiver->setLatestCmdId(cmd.cmd_id);
    m_testReceiver->setFormationTarget(cmd.formation_id, cmd.target_x, cmd.target_y,
                                       cmd.spacing, N);

    m_formationCompleted = false;

    std::string formationName = (formationIdx < 4)
        ? m_comboFormation->currentText().toStdString()
        : ("文字:\"" + cmd.formation_text + "\"");
    statusBar()->showMessage(QString::fromStdString(
        "已发送队形命令 (" + formationName + ")"));
    std::cout << "========== 切换队形 ==========" << std::endl;
}

void MainWindow::onLand() {
    DroneCmd cmd;
    cmd.cmd_id = m_cmdSender->getLastCmdId() + 1;
    cmd.cmd_type = DroneCmdType::CMD_LAND;
    cmd.expected_count = static_cast<uint32_t>(m_spinN->value());
    // ========== 加在 sendCommand 之前 ==========
    printf("[DEBUG] MainWindow发送前 formation_text 是否为空: %d\n", cmd.formation_text.empty());
    printf("[DEBUG] MainWindow发送前 内容: [%s]\n", cmd.formation_text.c_str());
    // ==========================================

    m_cmdSender->sendCommand(cmd);


    m_formationChecker->setLatestCmdId(cmd.cmd_id);
    m_stateReceiver->setLatestCmdId(cmd.cmd_id);
    m_testReceiver->setLatestCmdId(cmd.cmd_id);

    statusBar()->showMessage("已发送降落命令");
    std::cout << "========== 降落 ==========" << std::endl;
}

void MainWindow::onEmergency() {
    DroneCmd cmd = {};
    cmd.cmd_id = m_cmdSender->getLastCmdId() + 1;
    cmd.cmd_type = DroneCmdType::CMD_EMERGENCY;
    cmd.expected_count = static_cast<uint32_t>(m_spinN->value());
    // ========== 加在 sendCommand 之前 ==========
    printf("[DEBUG] MainWindow发送前 formation_text 是否为空: %d\n", cmd.formation_text.empty());
    printf("[DEBUG] MainWindow发送前 内容: [%s]\n", cmd.formation_text.c_str());
    // ==========================================

    m_cmdSender->sendCommand(cmd);


    m_formationChecker->setLatestCmdId(cmd.cmd_id);
    m_stateReceiver->setLatestCmdId(cmd.cmd_id);
    m_testReceiver->setLatestCmdId(cmd.cmd_id);

    statusBar()->showMessage("已发送急停命令");
    std::cout << "========== 急停 ==========" << std::endl;
}

void MainWindow::onTestModeToggled(bool checked) {
    if (checked) {
        m_fakeReceiver->stop();
        m_stateReceiver = m_testReceiver;
        m_stateReceiver->start();
        statusBar()->showMessage("测试模式：确定性队形");
        std::cout << "========== 切换到测试模式 ==========" << std::endl;
    } else {
        m_testReceiver->stop();
        m_stateReceiver = m_fakeReceiver;
        m_stateReceiver->start();
        statusBar()->showMessage("就绪");
        std::cout << "========== 切换到随机游走模式 ==========" << std::endl;
    }
}

void MainWindow::onCompletionCheck() {
    // Keep completion tracking active, but do not show transient progress in
    // the status bar. The quality panel is the single source of live progress.
    const auto progress = m_formationChecker->getProgress();
    const uint32_t totalCount = static_cast<uint32_t>(progress.second);

    if (m_formationChecker->isFormationComplete() && !m_formationCompleted) {
        m_formationCompleted = true;
        statusBar()->showMessage("队形完成！");
        statusBar()->setStyleSheet("QStatusBar{background-color:#4CAF50;color:white;font-weight:bold;}");
        std::cout << "[判定] 队形完成！" << std::endl;

        QMessageBox::information(this, "编队完成",
            QString::fromStdString("所有 " + std::to_string(totalCount) + " 架无人机已到达目标位置！"),
            QMessageBox::Ok);
    }
}

void MainWindow::onRenderTick() {
    updateDroneDisplay();
}

void MainWindow::updateDroneDisplay() {
    auto states = m_stateCache->getAllStates();

    m_coordMapper->autoFit(m_graphicsView, states);

    std::unordered_set<uint32_t> activeIds;

    for (const auto& state : states) {
        auto it = m_droneItems.find(state.drone_id);
        if (state.status == DroneStatus::ST_LANDED) {
            if (it != m_droneItems.end()) {
                it->second->updateFromState(state);
            }
            continue;
        }

        activeIds.insert(state.drone_id);
        if (it == m_droneItems.end()) {
            DroneItem* item = new DroneItem(state.drone_id);
            m_scene->addItem(item);
            m_droneItems[state.drone_id] = item;
            it = m_droneItems.find(state.drone_id);
        }
        it->second->updateFromState(state);
    }

    for (auto it = m_droneItems.begin(); it != m_droneItems.end(); ) {
        if (it->second->isLanding()) {
            if (it->second->advanceLandingAnimation()) {
                m_scene->removeItem(it->second);
                delete it->second;
                it = m_droneItems.erase(it);
            } else {
                ++it;
            }
            continue;
        }
        if (activeIds.find(it->first) == activeIds.end()) {
            m_scene->removeItem(it->second);
            delete it->second;
            it = m_droneItems.erase(it);
        } else {
            ++it;
        }
    }
}

void MainWindow::setupTelemetryPanel(QWidget* panel) {
    QGroupBox* group = new QGroupBox("通信性能", panel);
    group->setToolTip("统计 DroneState 的 DDS 业务数据，不含 DDS/UDP/IP 协议头和发现流量。");
    QVBoxLayout* layout = new QVBoxLayout(group);
    layout->setSpacing(10);

    QLabel* subtitle = new QLabel("实时 DDS 业务载荷（每秒窗口）", group);
    subtitle->setStyleSheet("QLabel{color:#6e8494;font-size:13px;}");
    layout->addWidget(subtitle);

    QGridLayout* metrics = new QGridLayout();
    metrics->setHorizontalSpacing(10);
    metrics->setVerticalSpacing(10);
    const QString valueStyle = "QLabel{background:#14242f;color:#d9f9f5;border:1px solid #294452;border-radius:5px;padding:7px 8px;font-weight:600;font-size:17px;}";
    const QString labelStyle = "QLabel{color:#7f96a6;font-size:13px;}";
    auto addMetric = [&](int row, const QString& label, QLabel*& value, const QString& tooltip) {
        QLabel* name = new QLabel(label, group);
        name->setStyleSheet(labelStyle);
        name->setToolTip(tooltip);
        value = new QLabel("—", group);
        value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        value->setStyleSheet(valueStyle);
        value->setToolTip(tooltip);
        metrics->addWidget(name, row, 0);
        metrics->addWidget(value, row, 1);
    };
    addMetric(0, "状态消息", m_lblMessageRate, "本秒收到的 DroneState 条目数。");
    addMetric(1, "业务吞吐", m_lblThroughput, "本秒 CDR 业务载荷，单位 kbit/s。协议头和发现流量不计入。");
    layout->addLayout(metrics);

    QLabel* chartTitle = new QLabel("吞吐趋势 · 最近 45 秒（kbit/s）", group);
    chartTitle->setStyleSheet("QLabel{color:#7f96a6;font-size:13px;margin-top:6px;}");
    layout->addWidget(chartTitle);
    m_telemetrySparkline = new TelemetrySparkline(group);
    m_telemetrySparkline->setToolTip("青色曲线表示最近 45 个一秒窗口的业务吞吐量。");
    m_telemetrySparkline->setFixedHeight(150);
    layout->addWidget(m_telemetrySparkline);

    QVBoxLayout* panelLayout = qobject_cast<QVBoxLayout*>(panel->layout());
    if (panelLayout) panelLayout->addWidget(group);
}

void MainWindow::setupQualityPanel(QWidget* panel) {
    QGroupBox* group = new QGroupBox("集群质量分析", panel);
    group->setToolTip("主控端基于已接收的实时状态计算。安全阈值仅用于展示，不参与 B 模块避障决策。");
    QVBoxLayout* layout = new QVBoxLayout(group);
    layout->setSpacing(10);

    QLabel* subtitle = new QLabel("主控状态几何统计", group);
    subtitle->setStyleSheet("QLabel{color:#6e8494;font-size:13px;}");
    layout->addWidget(subtitle);

    const QString labelStyle = "QLabel{color:#7f96a6;font-size:13px;}";
    const QString valueStyle = "QLabel{background:#14242f;color:#d9f9f5;border:1px solid #294452;border-radius:5px;padding:7px 8px;font-weight:600;font-size:17px;}";
    auto addMetric = [&](const QString& label, QLabel*& value, const QString& tooltip) {
        QHBoxLayout* row = new QHBoxLayout();
        QLabel* name = new QLabel(label, group);
        name->setStyleSheet(labelStyle);
        name->setToolTip(tooltip);
        value = new QLabel("—", group);
        value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        value->setStyleSheet(valueStyle);
        value->setToolTip(tooltip);
        row->addWidget(name);
        row->addWidget(value, 1);
        layout->addLayout(row);
    };
    addMetric("当前最小机间距", m_lblMinDistance,
              "每秒根据主控已接收的位置，计算任意两架无人机的最小欧氏距离。");
    addMetric("同步到达误差", m_lblSyncError,
               "主控记录每架机首次进入目标槽位的接收时间；显示最早与最晚到达的时间差。");
    addMetric("队形收敛残差", m_lblFormationResidual,
              "主控使用 B 模块的目标槽位，计算实际位置到各自目标槽位的平均欧氏距离。");

    QVBoxLayout* panelLayout = qobject_cast<QVBoxLayout*>(panel->layout());
    if (panelLayout) panelLayout->addWidget(group);
}

void MainWindow::setupStressTestPanel(QWidget* container) {
    QGroupBox* group = new QGroupBox("压力测试", container);
    group->setObjectName("stressGroup");
    QVBoxLayout* layout = new QVBoxLayout(group);

    m_comboStressPreset = new QComboBox();
    m_comboStressPreset->setToolTip("选择压力测试预设场景");
    auto presets = StressPreset::all();
    for (size_t i = 0; i < presets.size(); ++i) {
        m_comboStressPreset->addItem(QString::fromStdString(presets[i].name));
    }
    layout->addWidget(new QLabel("测试场景:"));
    layout->addWidget(m_comboStressPreset);

    m_lblStressStats = new QLabel("未启动");
    m_lblStressStats->setWordWrap(true);
    m_lblStressStats->setStyleSheet("QLabel{color:#888;font-size:11px;}");
    layout->addWidget(m_lblStressStats);

    connect(m_comboStressPreset, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onStressPresetChanged);

    QVBoxLayout* containerLayout = qobject_cast<QVBoxLayout*>(container->layout());
    if (containerLayout) {
        containerLayout->addWidget(group);
    }
}

void MainWindow::onStressPresetChanged(int index) {
    if (index < 0) return;
    auto presets = StressPreset::all();
    if (static_cast<size_t>(index) >= presets.size()) return;

    const StressPreset& preset = presets[index];

    m_fakeReceiver->stop();
    m_testReceiver->stop();
    m_stateCache->clear();

    m_coordMapper->reset();
    m_formationChecker->reset();
    m_formationChecker->setExpectedCount(preset.droneCount);
    m_formationChecker->setLatestCmdId(1);
    m_formationCompleted = false;

    m_stressReceiver->setPreset(preset);
    m_stressReceiver->start();
    m_stateReceiver = m_stressReceiver;

    m_spinN->blockSignals(true);
    m_spinN->setValue(static_cast<int>(preset.droneCount));
    m_spinN->blockSignals(false);

    m_chkTestMode->blockSignals(true);
    m_chkTestMode->setChecked(false);
    m_chkTestMode->blockSignals(false);

    statusBar()->showMessage(QString::fromStdString(
        "压力测试: " + preset.name + " | " + preset.description));
    std::cout << "========== 压力测试: " << preset.name << " ==========" << std::endl;
    std::cout << "[StressTest] " << preset.description << std::endl;
}

void MainWindow::updateStressStats() {
    if (m_stateReceiver != m_stressReceiver) {
        m_lblStressStats->setText("未启动");
        return;
    }
    if (!m_stressReceiver->currentPreset()) {
        m_lblStressStats->setText("未启动");
        return;
    }

    QString text;
    double targetHz = static_cast<double>(m_stressReceiver->currentPreset()->updateHz);
    double actualFps = m_stressReceiver->currentFps();
    bool fpsReady = (actualFps >= 0.0);
    bool isHealthy = fpsReady && (actualFps >= targetHz * 0.8);

    std::cout << "[StressStats] targetHz=" << targetHz
              << " actualFps=" << actualFps
              << " fpsReady=" << fpsReady
              << " isHealthy=" << isHealthy << std::endl;

    text += QString("场景: %1\n")
        .arg(QString::fromStdString(m_stressReceiver->currentPreset()->name));
    text += QString("无人机: %1 架\n")
        .arg(m_stressReceiver->currentPreset()->droneCount);
    text += QString("目标频率: %1 Hz\n")
        .arg(targetHz, 0, 'f', 0);
    text += QString("实际速率: %1\n")
        .arg(fpsReady ? QString::number(actualFps, 'f', 1) + " Hz" : "计算中...");
    text += QString("消息总数: %1\n")
        .arg(m_stressReceiver->messageCount());
    text += QString("状态: %1")
        .arg(!fpsReady ? "计算中..." : (isHealthy ? "正常" : "过载"));

    m_lblStressStats->setText(text);
    m_lblStressStats->setStyleSheet(
        !fpsReady ? "QLabel{color:#888;font-size:11px;}" :
        (isHealthy ? "QLabel{color:#4CAF50;font-size:11px;}" :
                     "QLabel{color:#F44336;font-size:11px;}"));
}

void MainWindow::updateCommunicationTelemetry() {
    detectTimedOutDrones();
    updateQualityMetrics();
    const CommunicationTelemetry telemetry = m_stateReceiver->takeCommunicationTelemetry();
    if (!telemetry.available) {
        m_lblMessageRate->setText("—");
        m_lblThroughput->setText("—");
        return;
    }

    const QLocale locale;
    const double kbitPerSecond = static_cast<double>(telemetry.payloadBytesPerSecond * 8ULL) / 1000.0;
    m_lblMessageRate->setText(locale.toString(static_cast<qulonglong>(telemetry.stateSamplesPerSecond)) + " /s");
    m_lblThroughput->setText(locale.toString(kbitPerSecond, 'f', 2) + " kbit/s");
    m_telemetrySparkline->appendSample(kbitPerSecond);
}

void MainWindow::detectTimedOutDrones() {
    if (!m_qualityTracking) return;

    // C detects a moving neighbor after three missed 10 Hz samples and a
    // hovering neighbor after three missed 1 Hz samples. The controller uses
    // a conservative grace period when the one-shot failover report is missed.
    constexpr auto kActiveGrace = std::chrono::milliseconds(1000);
    constexpr auto kHoverGrace = std::chrono::milliseconds(4500);
    const auto now = std::chrono::steady_clock::now();
    std::vector<uint32_t> timedOutIds;

    for (const auto& entry : m_lastSeenStates) {
        const uint32_t droneId = entry.first;
        if (m_lostDroneIds.count(droneId) != 0 ||
            m_qualityTargets.count(droneId) == 0) {
            continue;
        }

        const DroneStatus status = entry.second.status;
        const bool hovering = status == DroneStatus::ST_HOVER ||
                              status == DroneStatus::ST_IDLE;
        const auto grace = hovering ? kHoverGrace : kActiveGrace;
        if (now - entry.second.receivedAt > grace) {
            timedOutIds.push_back(droneId);
        }
    }

    for (uint32_t droneId : timedOutIds) {
        handleDroneLost(droneId);
    }
}

void MainWindow::updateQualityMetrics() {
    const std::vector<DroneState> allStates = m_stateCache->getAllStates();
    std::vector<DroneState> states;
    states.reserve(allStates.size());
    for (const DroneState& state : allStates) {
        // Planned landings are intentionally no longer part of the active
        // formation once the UI starts fading them out.
        if (state.status == DroneStatus::ST_LANDED) continue;
        if (m_qualityTracking &&
            m_qualityTargets.find(state.drone_id) == m_qualityTargets.end()) {
            continue;
        }
        states.push_back(state);
    }
    if (states.size() < 2) {
        m_lblMinDistance->setText("—");
        m_lblSyncError->setText(m_qualityTracking ? "等待状态" : "等待队形命令");
        m_lblFormationResidual->setText(m_qualityTracking ? "等待状态" : "等待队形命令");
        return;
    }

    double minDistance = std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < states.size(); ++i) {
        for (size_t j = i + 1; j < states.size(); ++j) {
            const double dx = states[i].x - states[j].x;
            const double dy = states[i].y - states[j].y;
            minDistance = std::min(minDistance, std::sqrt(dx * dx + dy * dy));
        }
    }

    m_lblMinDistance->setText(QString::number(minDistance, 'f', 2) + " m");
    m_lblMinDistance->setStyleSheet(metricStyle(minDistance < 1.50));

    if (!m_qualityTracking || m_qualityTargets.empty()) {
        m_lblSyncError->setText("等待队形命令");
        m_lblFormationResidual->setText("等待队形命令");
        return;
    }

    double totalResidual = 0.0;
    uint32_t matchedCount = 0;
    for (const DroneState& state : states) {
        const auto target = m_qualityTargets.find(state.drone_id);
        if (target == m_qualityTargets.end()) continue;
        const double dx = state.x - target->second.x;
        const double dy = state.y - target->second.y;
        totalResidual += std::sqrt(dx * dx + dy * dy);
        ++matchedCount;
    }

    if (matchedCount == 0) {
        m_lblFormationResidual->setText("等待匹配状态");
    } else {
        const double averageResidual = totalResidual / matchedCount;
        m_lblFormationResidual->setText(QString::number(averageResidual, 'f', 2) + " m (平均)");
        m_lblFormationResidual->setStyleSheet(metricStyle(averageResidual > kArrivalThresholdMeters));
    }

    const uint32_t expectedCount = static_cast<uint32_t>(m_qualityTargets.size());
    if (m_arrivalTimes.size() < expectedCount) {
        m_lblSyncError->setText(QString("到达 %1/%2").arg(m_arrivalTimes.size()).arg(expectedCount));
        m_lblSyncError->setStyleSheet(metricStyle());
        return;
    }

    auto earliest = m_arrivalTimes.begin()->second;
    auto latest = earliest;
    for (const auto& arrival : m_arrivalTimes) {
        earliest = std::min(earliest, arrival.second);
        latest = std::max(latest, arrival.second);
    }
    const double syncError = std::chrono::duration<double>(latest - earliest).count();
    m_lblSyncError->setText(QString::number(syncError, 'f', 2) + " s");
    m_lblSyncError->setStyleSheet(metricStyle());
}

bool MainWindow::beginQualityTracking(const DroneCmd& cmd) {
    // A takeoff after landing starts a fresh mission. Do not carry timeout
    // or failover records from the previous swarm instance into its metrics.
    if (cmd.cmd_type == DroneCmdType::CMD_TAKEOFF) {
        m_lostDroneIds.clear();
        m_lastSeenStates.clear();
    }

    m_qualityPlan.formationId = (cmd.cmd_type == DroneCmdType::CMD_TAKEOFF && cmd.formation_id == 0)
        ? 0 : cmd.formation_id;
    m_qualityPlan.formationText = cmd.formation_text;
    m_qualityPlan.centerX = cmd.target_x;
    m_qualityPlan.centerY = cmd.target_y;
    m_qualityPlan.spacing = cmd.spacing;

    m_qualityActiveDroneIds.clear();
    for (uint32_t droneId = 1; droneId <= cmd.expected_count; ++droneId) {
        if (m_lostDroneIds.count(droneId) == 0) {
            m_qualityActiveDroneIds.push_back(droneId);
        }
    }

    m_arrivalTimes.clear();
    m_qualityCmdId = cmd.cmd_id;
    m_qualityTracking = rebuildQualityTargets();
    return m_qualityTracking;
}

bool MainWindow::rebuildQualityTargets() {
    if (m_qualityActiveDroneIds.empty()) {
        m_qualityTargets.clear();
        m_arrivalTimes.clear();
        return false;
    }

    std::sort(m_qualityActiveDroneIds.begin(), m_qualityActiveDroneIds.end());

    SwarmFormation::FormationRequest request;
    request.formation_id = m_qualityPlan.formationId;
    request.formation_text = m_qualityPlan.formationText;
    request.count = static_cast<uint32_t>(m_qualityActiveDroneIds.size());
    request.center_x = m_qualityPlan.centerX;
    request.center_y = m_qualityPlan.centerY;
    request.spacing = m_qualityPlan.spacing;

    const SwarmFormation::FormationResult result = SwarmFormation::GenerateSlots(request);
    if (!result.success) {
        m_qualityTargets.clear();
        m_arrivalTimes.clear();
        std::cerr << "[Quality] 目标槽位计算失败: "
                  << SwarmFormation::FormationErrorMessage(result.error) << std::endl;
        return false;
    }

    std::unordered_map<uint32_t, TargetSlot> rebuiltTargets;
    for (uint32_t index = 0; index < request.count; ++index) {
        SwarmFormation::Point2D slot;
        if (!SwarmFormation::GetSlot(result, index, slot)) {
            m_qualityTargets.clear();
            m_arrivalTimes.clear();
            std::cerr << "[Quality] 读取目标槽位失败，index=" << index << std::endl;
            return false;
        }
        // C uses the sorted alive-ID list to map new slot indexes after a loss.
        rebuiltTargets.emplace(m_qualityActiveDroneIds[index], TargetSlot{slot.x, slot.y});
    }

    m_qualityTargets = std::move(rebuiltTargets);
    m_arrivalTimes.clear();
    return true;
}

void MainWindow::handleDroneLost(uint32_t droneId) {
    if (droneId == 0 || !m_lostDroneIds.insert(droneId).second) return;

    m_stateCache->removeState(droneId);
    if (!m_qualityTracking) {
        statusBar()->showMessage(QString("检测到无人机 %1 失联").arg(droneId));
        return;
    }

    const auto active = std::find(m_qualityActiveDroneIds.begin(),
                                  m_qualityActiveDroneIds.end(), droneId);
    if (active == m_qualityActiveDroneIds.end()) return;

    m_qualityActiveDroneIds.erase(active);
    m_arrivalTimes.clear();
    m_formationCompleted = false;
    m_formationChecker->setExpectedCount(
        static_cast<uint32_t>(m_qualityActiveDroneIds.size()));

    if (!rebuildQualityTargets()) {
        m_qualityTracking = false;
        statusBar()->showMessage("失联后无法重新计算质量指标");
        return;
    }

    statusBar()->showMessage(QString("检测到无人机 %1 失联，按 %2 架存活机重新统计")
                             .arg(droneId)
                             .arg(m_qualityActiveDroneIds.size()));
}

void MainWindow::observeStateForQuality(const DroneState& state) {
    m_lastSeenStates[state.drone_id] = LastSeenState{
        std::chrono::steady_clock::now(), state.status};

    if (!m_qualityTracking || state.current_cmd_id != m_qualityCmdId) return;

    const auto target = m_qualityTargets.find(state.drone_id);
    if (target == m_qualityTargets.end() || m_arrivalTimes.count(state.drone_id) != 0) return;

    const double dx = state.x - target->second.x;
    const double dy = state.y - target->second.y;
    if (std::sqrt(dx * dx + dy * dy) <= kArrivalThresholdMeters) {
        m_arrivalTimes.emplace(state.drone_id, std::chrono::steady_clock::now());
    }
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_graphicsView->viewport()) {
        if (event->type() == QEvent::Wheel) {
            QWheelEvent* wheel = static_cast<QWheelEvent*>(event);
            const double scaleFactor = 1.15;
            if (wheel->angleDelta().y() > 0) {
                m_graphicsView->scale(scaleFactor, scaleFactor);
            } else {
                m_graphicsView->scale(1.0 / scaleFactor, 1.0 / scaleFactor);
            }
            return true;
        }
        if (event->type() == QEvent::MouseButtonDblClick) {
            m_coordMapper->resetView(m_graphicsView);
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
