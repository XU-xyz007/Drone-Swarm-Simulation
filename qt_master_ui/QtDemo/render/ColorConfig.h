#pragma once
#include <QColor>
#include "core/DroneState.h"

inline QColor colorForStatus(DroneStatus status) {
    switch (status) {
        case DroneStatus::ST_IDLE:    return QColor(76, 175, 80);   // 绿色：待命/已到位
        case DroneStatus::ST_TAKEOFF: return QColor(255, 193, 7);   // 琥珀色：起飞中
        case DroneStatus::ST_CRUISE:  return QColor(255, 235, 59);  // 黄色：移动中
        case DroneStatus::ST_LANDING: return QColor(33, 150, 243);  // 蓝色：降落中
        case DroneStatus::ST_LANDED:  return QColor(156, 39, 176);  // 紫色：已降落
        case DroneStatus::ST_FAULT:   return QColor(244, 67, 54);   // 红色：故障
        case DroneStatus::ST_LOST:    return QColor(128, 128, 128); // 灰色：失联
        default:                      return QColor(255, 255, 255);
    }
}