cmake_minimum_required(VERSION 3.20)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Algorithm/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Algorithm/Formations)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Algorithm/Math)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
)

add_executable(QtDemo
    QtDemo.cpp
    MainWindow.h
    MainWindow.cpp
    "core/DroneCmd.h"
    "core/DroneState.h"
    "core/ICommandSender.h"
    "core/IStateReceiver.h"
    "core/IDroneStateCache.h"
    "core/IFormationChecker.h"
    "core/DroneStateCache.h"
    "core/DroneStateCache.cpp"
    "core/FormationChecker.h"
    "core/FormationChecker.cpp"
    "fake/FakeCommandSender.h"
    "fake/FakeCommandSender.cpp"
    "fake/FakeStateReceiver.h"
    "fake/FakeStateReceiver.cpp"
    "render/ColorConfig.h"
    "render/DroneItem.h"
    "render/DroneItem.cpp"
    "render/CoordinateMapper.h"
    "render/CoordinateMapper.cpp"
    "render/GridScene.h"
    "render/GridScene.cpp"
    "render/TelemetrySparkline.h"
    "../../Algorithm/FormationAlgorithm.cpp"
    "../../Algorithm/Math/GeometryUtils.cpp"
    "../../Algorithm/Formations/HersheyFont.cpp"
    "../../Algorithm/Formations/PresetFormations.cpp"
    "../../Algorithm/Formations/TextFormation.cpp"
    "../../Algorithm/Formations/TextRasterizerWin.cpp"
)

target_link_libraries(QtDemo
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)