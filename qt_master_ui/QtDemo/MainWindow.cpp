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
    , m_stateCache(stateCache)
    , m_formationChecker(formationChecker)
{
    setWindowTitle("无人机编队主控端");
    resize(1160, 760);

    m_coordMapper = new CoordinateMapper();

    setupUi();
    setupStatusBar();

    m_completionTimer = new QTimer(this);
    connect(m_completionTimer, &QTimer::timeout, this, &MainWindow::onCompletionCheck);
    m_completionTimer->start(200);

    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, &MainWindow::onRenderTick);
    m_renderTimer->start(50);

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
    m_spinN->setRange(1, 200);
    m_spinN->setValue(8);
    m_spinN->setToolTip("无人机数量 (1~200)");
    form->addRow("无人机数量 N:", m_spinN);

    connect(m_spinN, QOverload<int>::of(&QSpinBox::valueChanged),
        [this](int value) {
            std::cout << "[UI] 无人机数量变更为: " << value << std::endl;
            m_stateReceiver->setDroneCount(static_cast<uint32_t>(value));
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
    m_spinSpacing->setRange(3.0, 100.0);
    m_spinSpacing->setToolTip(QString::fromUtf8("最小间距 3.0m，低于此值可能与碰撞避免安全距离冲突"));
    m_spinSpacing->setValue(5.0);
    m_spinSpacing->setDecimals(1);
    m_spinSpacing->setSuffix(" m");
    form->addRow("间距:", m_spinSpacing);

    QVBoxLayout* panelLayout = qobject_cast<QVBoxLayout*>(panel->layout());
    if (panelLayout) {
        panelLayout->insertWidget(0, group);
    }
}

void MainWindow::setupCanvas() {
    m_scene = new GridScene(this);
    m_scene->setSceneRect(-60, -50, 120, 100);

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

    statusBar()->showMessage("已发送急停命令");
    std::cout << "========== 急停 ==========" << std::endl;
}

void MainWindow::onCompletionCheck() {
    auto progress = m_formationChecker->getProgress();
    uint32_t totalCount = static_cast<uint32_t>(progress.second);

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
        if (state.status == DroneStatus::ST_LANDED || state.status == DroneStatus::ST_LOST) continue;

        activeIds.insert(state.drone_id);

        auto it = m_droneItems.find(state.drone_id);
        if (it == m_droneItems.end()) {
            DroneItem* item = new DroneItem(state.drone_id);
            m_scene->addItem(item);
            m_droneItems[state.drone_id] = item;
            it = m_droneItems.find(state.drone_id);
        }
        it->second->updateFromState(state);
    }

    for (auto it = m_droneItems.begin(); it != m_droneItems.end(); ) {
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

void MainWindow::updateCommunicationTelemetry() {
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

void MainWindow::updateQualityMetrics() {
    const std::vector<DroneState> states = m_stateCache->getAllStates();
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
    SwarmFormation::FormationRequest request;
    request.formation_id = (cmd.cmd_type == DroneCmdType::CMD_TAKEOFF && cmd.formation_id == 0)
        ? 0 : cmd.formation_id;
    request.formation_text = cmd.formation_text;
    request.count = cmd.expected_count;
    request.center_x = cmd.target_x;
    request.center_y = cmd.target_y;
    request.spacing = cmd.spacing;

    const SwarmFormation::FormationResult result = SwarmFormation::GenerateSlots(request);
    if (!result.success) {
        m_qualityTargets.clear();
        m_arrivalTimes.clear();
        m_qualityTracking = false;
        std::cerr << "[Quality] 目标槽位计算失败: "
                  << SwarmFormation::FormationErrorMessage(result.error) << std::endl;
        return false;
    }

    m_qualityTargets.clear();
    for (uint32_t index = 0; index < request.count; ++index) {
        // C 模块的约定：drone_id 1..N 对应 B 算法的 slot index 0..N-1。
        SwarmFormation::Point2D slot;
        if (!SwarmFormation::GetSlot(result, index, slot)) {
            m_qualityTargets.clear();
            m_arrivalTimes.clear();
            m_qualityTracking = false;
            std::cerr << "[Quality] 读取目标槽位失败，index=" << index << std::endl;
            return false;
        }
        m_qualityTargets.emplace(index + 1, TargetSlot{slot.x, slot.y});
    }
    m_arrivalTimes.clear();
    m_qualityCmdId = cmd.cmd_id;
    m_qualityTracking = true;
    return true;
}

void MainWindow::observeStateForQuality(const DroneState& state) {
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