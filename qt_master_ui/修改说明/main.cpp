#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QMetaObject>
#include <iostream>
#include <cstring>
#include <windows.h>


// --- 引入 A 模块 (UI) 的核心文件 ---
#include "../qt_master_ui/QtDemo/MainWindow.h"
#include "../qt_master_ui/QtDemo/core/DroneStateCache.h"
#include "../qt_master_ui/QtDemo/core/FormationChecker.h"
#include "../qt_master_ui/QtDemo/core/ICommandSender.h"
#include "../qt_master_ui/QtDemo/core/IStateReceiver.h"

// --- 引入 D 模块 (通信) 的头文件 ---
#include "../Communication/DdsService.h"

// ======================================================================
// 1. 发送适配器：将 UI 的发送请求转换为 DDS 的 Publish
// ======================================================================
class RealCommandSender : public ICommandSender {
public:
    // 签名与基类完全一致，使用全局 DroneCmd 类型
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

        bool ret = SwarmComm::PublishCmd(ddsCmd);
        if (!ret) {
            std::cerr << "[DDS Error] 命令发送失败！CMD_ID: " << cmd.cmd_id << std::endl;
        }
        return ret;
    }


    uint32_t getLastCmdId() const override {
        return m_lastCmdId;
    }

private:
    uint32_t m_lastCmdId = 0;
};

// ======================================================================
// 2. 接收适配器：将 DDS 的后台回调安全地抛回 Qt UI 主线程
// ======================================================================
class RealStateReceiver : public IStateReceiver {
public:
    void setOnStateReceived(std::function<void(const DroneState&)> callback) override {
        m_onStateReceived = callback;
    }

    void setOnDroneRemoved(std::function<void(uint32_t)> callback) override {
        m_onDroneRemoved = callback;
    }

    void setLatestCmdId(uint32_t cmdId) override {
        m_latestCmdId = cmdId;
    }

    void start() override {
        // 订阅底层状态，回调参数是 SharedMessage 命名空间下的类型
        SwarmComm::SubscribeState([this](const SharedMessage::DroneState& ddsState) {
            // ========== 加这里：验证DDS回调有没有进到这里 ==========
            printf("[DEBUG][回调入口] 收到无人机%d DDS坐标: (%.2f, %.2f)\n", ddsState.drone_id, ddsState.x, ddsState.y);
            // 跨线程抛到 UI 主线程
            QMetaObject::invokeMethod(qApp, [this, ddsState]() {
                // ========== 加这里：验证数据有没有成功到UI主线程 ==========
                printf("[DEBUG][UI线程] 处理无人机%d DDS坐标: (%.2f, %.2f)\n", ddsState.drone_id, ddsState.x, ddsState.y);
                printf("[调试] 回调是否有效：%d\n", (bool)m_onStateReceived);
                if (m_onStateReceived) {
                    // 类型转换：转回基类接口使用的全局类型
                    DroneState state;
                    state.drone_id = ddsState.drone_id;
                    state.x = ddsState.x;
                    state.y = ddsState.y;
                    state.status = static_cast<DroneStatus>(ddsState.status);
                    state.current_cmd_id = ddsState.current_cmd_id;
                    state.transitioning = false; // DDS没传，业务层自己给默认值
                    m_onStateReceived(state);
                    printf("[调试] 回调调用完成\n");

                }
                }, Qt::QueuedConnection);
            });
        std::cout << "[DDS] 状态订阅已启动。" << std::endl;
    }

    void stop() override {
        // V1 版本 DdsService 暂无 Unsubscribe，留空即可
    }

private:
    std::function<void(const DroneState&)> m_onStateReceived;
    std::function<void(uint32_t)> m_onDroneRemoved;
    uint32_t m_latestCmdId = 0;
};

// ======================================================================
// 3. 程序主入口
// ======================================================================
int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);
    QApplication a(argc, argv);

    // 【安全检查 1】确保 QoS 配置文件存在
    if (!QFileInfo::exists("ZRDDS_QOS_PROFILES.xml")) {
        QMessageBox::critical(nullptr, "启动失败", "缺失 ZRDDS_QOS_PROFILES.xml 配置文件！\n请检查 x64/Debug 目录。");
        return -1;
    }

    // 【安全检查 2】确保 License 文件存在
    if (!QFileInfo::exists("zrddslicence.lic")) {
        QMessageBox::critical(nullptr, "启动失败", "缺失 zrddslicence.lic 授权文件！\n请检查 x64/Debug 目录。");
        return -1;
    }

    // 【底层预热】初始化 DDS
    if (!SwarmComm::InitDDS()) {
        QMessageBox::critical(nullptr, "致命错误", "ZR-DDS 中间件初始化失败！");
        return -1;
    }

    // 【依赖注入】组装三大模块
    RealCommandSender realCmdSender;
    RealStateReceiver realStateReceiver;
    DroneStateCache stateCache;
    FormationChecker formationChecker;

    // ========== 新增这一行：把状态接收器和状态缓存打通 ==========
    realStateReceiver.setOnStateReceived(
        [&](const DroneState& state) {
            stateCache.updateState(state);
            formationChecker.updateDroneCmdId(state.drone_id, state.current_cmd_id);
            formationChecker.updateDroneStatus(state.drone_id, state.status);
        }
    );

    MainWindow w(&realCmdSender, &realStateReceiver, &stateCache, &formationChecker);

    // 启动后台接收
    realStateReceiver.start();

    // 显示界面
    w.show();

    // 进入 Qt 事件主循环
    int execCode = a.exec();

    // 【资源清理】安全关闭 DDS
    SwarmComm::ShutdownDDS();

    return execCode;
}