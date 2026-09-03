#include <QApplication>
#include <QTimer>
#include <QTextStream>
#include <iostream>
#include <memory>
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
#include "dds/DdsCommandSender.h"
#include "dds/DdsStateReceiver.h"


int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    const QStringList arguments = app.arguments();
    const bool ddsMode = arguments.contains("--dds");
    const int ddsIndex = arguments.indexOf("--dds");
    bool countOk = false;
    const uint32_t N = (ddsMode && ddsIndex + 1 < arguments.size())
        ? arguments.at(ddsIndex + 1).toUInt(&countOk) : 8;
    const uint32_t droneCount = countOk ? N : 8;

    std::unique_ptr<ICommandSender> sender;
    std::unique_ptr<IStateReceiver> receiver;
    if (ddsMode) {
        sender = std::make_unique<DdsCommandSender>();
        receiver = std::make_unique<DdsStateReceiver>();
        std::cout << "DDS UI mode enabled; expecting " << droneCount << " drones." << std::endl;
    } else {
        sender = std::make_unique<FakeCommandSender>();
        receiver = std::make_unique<FakeStateReceiver>();
        std::cout << "Fake UI mode enabled. Use --dds [count] for real ZR-DDS states." << std::endl;
    }
    DroneStateCache stateCache;
    FormationChecker formationChecker;

    receiver->setDroneCount(droneCount);

    // 注册状态回调：收到 DroneState 后更新缓存 + 检查队形完成
    receiver->setOnStateReceived([&](const DroneState& state) {
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

    receiver->setOnDroneRemoved([&](uint32_t droneId) {
        stateCache.removeState(droneId);
    });

    receiver->start();

    // 创建主窗口（注入所有依赖）
    MainWindow window(sender.get(), receiver.get(), &stateCache, &formationChecker);
    window.show();

    return app.exec();
}
