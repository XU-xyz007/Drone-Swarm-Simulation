cmake_minimum_required(VERSION 3.20)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

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
    "dds/DdsCommandSender.h"
    "dds/DdsCommandSender.cpp"
    "dds/DdsStateReceiver.h"
    "dds/DdsStateReceiver.cpp"
    "../../Communication/DdsService.h"
    "../../Communication/DdsService.cpp"
    "../../SharedMessage/gen_code/DroneMessage.cpp"
    "../../SharedMessage/gen_code/DroneMessageTypeSupport.cpp"
    "render/ColorConfig.h"
    "render/DroneItem.h"
    "render/DroneItem.cpp"
    "render/CoordinateMapper.h"
    "render/CoordinateMapper.cpp"
    "test/TestStateReceiver.h"
    "test/TestStateReceiver.cpp"
)

target_link_libraries(QtDemo PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)

if (WIN32)
    if (NOT DEFINED ENV{ZRDDS_HOME})
        message(FATAL_ERROR "ZRDDS_HOME is required to build the DDS UI mode.")
    endif()
    target_compile_definitions(QtDemo PRIVATE _ZRDDSDLLIMPORT _ZRDDSCPPINTERFACE)
    target_include_directories(QtDemo PRIVATE
        "$ENV{ZRDDS_HOME}/include/ZRDDSCoreInterface"
        "$ENV{ZRDDS_HOME}/include/CPlusPlusInterface"
        "../../SharedMessage/gen_code"
    )
    target_link_directories(QtDemo PRIVATE "$ENV{ZRDDS_HOME}/lib")
    target_link_libraries(QtDemo PRIVATE
        debug ZRDDSCppd_VS2015
        optimized ZRDDSCpp_VS2015
    )
    add_custom_command(TARGET QtDemo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/../../ZRDDS_QOS_PROFILES.xml" "$<TARGET_FILE_DIR:QtDemo>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$ENV{ZRDDS_HOME}/lib/ZRDDSCppd_VS2015.dll" "$<TARGET_FILE_DIR:QtDemo>"
    )
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../zrddslicence.lic")
        add_custom_command(TARGET QtDemo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/../../zrddslicence.lic" "$<TARGET_FILE_DIR:QtDemo>"
        )
    else()
        message(WARNING "zrddslicence.lic is not in the repository; copy it beside QtDemo.exe before running DDS mode.")
    endif()
endif()
