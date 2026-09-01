#include <QApplication>
#include <QTimer>
#include <QTextStream>
#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#endif

#include "MainWindow.h"
#include "core/DroneState.h"
#include "core/DroneCmd.h"
#include "core/DroneStateCache.h"
#include "core/FormationChecker.h"
#include "fake/FakeCommandSender.h"
#include "fake/FakeStateReceiver.h"


int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 创建 Fake 实现
    FakeCommandSender sender;
    FakeStateReceiver receiver;
    DroneStateCache stateCache;
    FormationChecker formationChecker;

    const uint32_t N = 8;
    receiver.setDroneCount(N);

    // 注册状态回调：收到 DroneState 后更新缓存 + 检查队形完成
    receiver.setOnStateReceived([&](const DroneState& state) {
        stateCache.updateState(state);
        formationChecker.updateDroneCmdId(state.drone_id, state.current_cmd_id);
        formationChecker.updateDroneStatus(state.drone_id, state.status);

        static int frameCount = 0;
        if (++frameCount % 10 == 0) {
            auto progress = formationChecker.getProgress();
            std::cout << "[状态] 在线: " << stateCache.getDroneCount()
                      << " 架 | 队形进度: " << progress.first << "/" << progress.second
                      << std::endl;
        }
    });

    receiver.setOnDroneRemoved([&](uint32_t droneId) {
        stateCache.removeState(droneId);
    });

    receiver.start();
    std::cout << "Fake 状态接收器已启动 (10Hz, N=" << N << ")" << std::endl;

    // 创建主窗口（注入所有依赖）
    MainWindow window(&sender, &receiver, &stateCache, &formationChecker);
    window.show();

    return app.exec();
}