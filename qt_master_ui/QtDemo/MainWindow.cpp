#include "MainWindow.h"
#include "core/ICommandSender.h"
#include "core/IStateReceiver.h"
#include "core/IDroneStateCache.h"
#include "core/IFormationChecker.h"
#include "core/DroneCmd.h"
#include "core/DroneState.h"
#include "render/DroneItem.h"
#include "render/ColorConfig.h"
#include "render/CoordinateMapper.h"
#include "test/TestStateReceiver.h"
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QWheelEvent>
#include <QScrollBar>
#include <iostream>
#include <unordered_set>

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
    resize(1000, 700);

    m_testReceiver = new TestStateReceiver(this);
    m_testReceiver->setDroneCount(8);
    m_testReceiver->setOnStateReceived([this](const DroneState& state) {
        m_stateCache->updateState(state);
        m_formationChecker->updateDroneCmdId(state.drone_id, state.current_cmd_id);
        m_formationChecker->updateDroneStatus(state.drone_id, state.status);
    });
    m_testReceiver->setOnDroneRemoved([this](uint32_t droneId) {
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
    m_renderMetricsTimer.start();
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
            m_fakeReceiver->setDroneCount(static_cast<uint32_t>(value));
            m_testReceiver->setDroneCount(static_cast<uint32_t>(value));
        });

    m_comboFormation = new QComboBox();
    m_comboFormation->addItem("0 - 方阵");
    m_comboFormation->addItem("1 - 一字");
    m_comboFormation->addItem("2 - 三角");
    m_comboFormation->addItem("3 - 圆形");
    m_comboFormation->addItem("4 - 字母D");
    m_comboFormation->addItem("5 - 字母S");
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
                m_editFormationText->setEnabled(index == 6);
            });

    m_spinCenterX = new QDoubleSpinBox();
    m_spinCenterX->setRange(-1000.0, 1000.0);
    m_spinCenterX->setValue(0.0);
    m_spinCenterX->setDecimals(1);
    m_spinCenterX->setSuffix(" m");
    form->addRow("中心 X:", m_spinCenterX);

    m_spinCenterY = new QDoubleSpinBox();
    m_spinCenterY->setRange(-1000.0, 1000.0);
    m_spinCenterY->setValue(0.0);
    m_spinCenterY->setDecimals(1);
    m_spinCenterY->setSuffix(" m");
    form->addRow("中心 Y:", m_spinCenterY);

    m_spinSpacing = new QDoubleSpinBox();
    m_spinSpacing->setRange(0.5, 100.0);
    m_spinSpacing->setValue(30.0);
    m_spinSpacing->setDecimals(1);
    m_spinSpacing->setSuffix(" m");
    form->addRow("间距:", m_spinSpacing);

    m_chkTestMode = new QCheckBox("测试模式（确定性队形）");
    m_chkTestMode->setToolTip("勾选后无人机按确定队形目标飞行，可验证队形完成判定");
    connect(m_chkTestMode, &QCheckBox::toggled, this, &MainWindow::onTestModeToggled);
    form->addRow(m_chkTestMode);

    QVBoxLayout* panelLayout = qobject_cast<QVBoxLayout*>(panel->layout());
    if (panelLayout) {
        panelLayout->insertWidget(0, group);
    }
}

void MainWindow::setupCanvas() {
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(-200, -200, 400, 400);

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
    DroneCmd cmd;
    cmd.cmd_id = m_cmdSender->getLastCmdId() + 1;
    cmd.cmd_type = DroneCmdType::CMD_TAKEOFF;
    cmd.expected_count = static_cast<uint32_t>(m_spinN->value());
    m_cmdSender->sendCommand(cmd);
    statusBar()->showMessage("已发送起飞命令");
    std::cout << "========== 起飞 ==========" << std::endl;
}

void MainWindow::onFormation() {
    int formationIdx = m_comboFormation->currentIndex();
    DroneCmd cmd;
    cmd.cmd_id = m_cmdSender->getLastCmdId() + 1;
    cmd.cmd_type = DroneCmdType::CMD_FORMATION;
    cmd.target_x = m_spinCenterX->value();
    cmd.target_y = m_spinCenterY->value();
    cmd.spacing = m_spinSpacing->value();
    cmd.expected_count = static_cast<uint32_t>(m_spinN->value());

    if (formationIdx == 6) {
        QString text = m_editFormationText->text().trimmed().toUpper();
        if (text == "D") {
            cmd.formation_id = 4;
        } else if (text == "S") {
            cmd.formation_id = 5;
        } else {
            statusBar()->showMessage("文字队形 \"" + text + "\" 暂无");
            std::cout << "[UI] 文字队形 \"" << text.toStdString() << "\" 暂无" << std::endl;
            return;
        }
    } else {
        cmd.formation_id = (formationIdx < 6) ? static_cast<uint32_t>(formationIdx) : 0;
        cmd.formation_text = "";
    }

    m_cmdSender->sendCommand(cmd);

    uint32_t N = static_cast<uint32_t>(m_spinN->value());
    m_formationChecker->setExpectedCount(N);
    m_formationChecker->setLatestCmdId(cmd.cmd_id);
    m_stateReceiver->setLatestCmdId(cmd.cmd_id);

    m_testReceiver->setLatestCmdId(cmd.cmd_id);
    m_testReceiver->setFormationTarget(cmd.formation_id, cmd.target_x, cmd.target_y,
                                       cmd.spacing, N);

    m_formationCompleted = false;

    std::string formationName = (formationIdx < 6)
        ? m_comboFormation->currentText().toStdString()
        : ("文字:\"" + cmd.formation_text + "\"");
    statusBar()->showMessage(QString::fromStdString(
        "已发送队形命令 (" + formationName + ")，进度: 0/" + std::to_string(N)));
    std::cout << "========== 切换队形 ==========" << std::endl;
}

void MainWindow::onLand() {
    DroneCmd cmd;
    cmd.cmd_id = m_cmdSender->getLastCmdId() + 1;
    cmd.cmd_type = DroneCmdType::CMD_LAND;
    cmd.expected_count = static_cast<uint32_t>(m_spinN->value());
    m_cmdSender->sendCommand(cmd);
    statusBar()->showMessage("已发送降落命令");
    std::cout << "========== 降落 ==========" << std::endl;
}

void MainWindow::onEmergency() {
    DroneCmd cmd;
    cmd.cmd_id = m_cmdSender->getLastCmdId() + 1;
    cmd.cmd_type = DroneCmdType::CMD_EMERGENCY;
    cmd.expected_count = static_cast<uint32_t>(m_spinN->value());
    m_cmdSender->sendCommand(cmd);
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
    auto progress = m_formationChecker->getProgress();
    uint32_t totalCount = static_cast<uint32_t>(m_spinN->value());

    if (!m_formationCompleted) {
        statusBar()->showMessage(QString::fromStdString(
            "队形进行中，进度: " + std::to_string(progress.first) + "/" + std::to_string(totalCount)));
    }

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
    ++m_renderFramesInWindow;
    if (m_renderMetricsTimer.elapsed() >= 1000) {
        std::cout << "[UI-Metrics] render=" << m_renderFramesInWindow
                  << " frames/s | cached drones=" << m_stateCache->getDroneCount()
                  << std::endl;
        m_renderFramesInWindow = 0;
        m_renderMetricsTimer.restart();
    }
}

void MainWindow::updateDroneDisplay() {
    auto states = m_stateCache->getAllStates();

    if (states.empty()) return;

    m_coordMapper->autoFit(m_graphicsView, states);

    std::unordered_set<uint32_t> activeIds;

    for (const auto& state : states) {
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
