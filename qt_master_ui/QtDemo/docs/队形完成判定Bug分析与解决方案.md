# 队形完成判定 Bug 分析与解决方案

---

## 一、问题现象

**复现步骤**：
1. 测试模式，设置 N=50，中心(0,0)，切换方阵队形 → 50 架无人机到达目标，全部变绿，提示"队形完成"
2. 修改中心 Y=100，切换队形 → 50 架无人机变黄，飞向新目标，到达后变绿，提示"队形完成"
3. 修改 N=8，中心 Y=0，切换队形 → **8 架无人机还在飞行中（黄色），就弹出了"队形完成"提示**

**预期行为**：8 架无人机全部到达目标位置（变绿）后，才弹出"队形完成"。

---

## 二、根因分析

### 2.1 数据流追踪

```
用户点击"切换队形"
  → MainWindow::onFormation()
    → setDroneCount(8)          // 触发：42 架标记为 flyOutDrone = true
    → setFormationTarget(...)   // 计算 8 架的目标位置
    → FormationChecker.setExpectedCount(8)
    → FormationChecker.setLatestCmdId(cmd_3)

onTimerTick (100ms)
  → 8 架活跃无人机：m_reached=false → ST_CRUISE（黄色）
  → 42 架飞出无人机：m_flyOutDrone=true → 当前代码中 m_reached 未被重置 → ST_IDLE（绿色）
  → callback → FormationChecker.updateDroneStatus(droneId, status)

onCompletionCheck (200ms)
  → FormationChecker.isFormationComplete()
    → countIdleLocked() = 42（飞出无人机都是 ST_IDLE） >= 8（expectedCount）→ 通过 ✓
    → 但 cmd_id 检查：42 架的 cmd_id 是旧的 cmd_2 != cmd_3 → 不通过 ✗
  → 如果 42 架被 removeDrone 清理了 → m_droneCmdIds 只剩 8 架
    → 8 架都是旧 cmd_id → 不通过 ✗
```

### 2.2 真正的问题

FormationChecker 的判定使用 `countIdleLocked() >= m_expectedCount`，这是**"大于等于"语义**。它统计的是**全局所有 ST_IDLE 无人机**的数量，而不是"属于当前编队的 N 架中已到达的数量"。

当无人机数量从 50 缩减到 8 时：
- 42 架飞出无人机应当**不参与队形完成判定**
- 但 FormationChecker 不知道哪些无人机是"当前编队成员"，哪些是"已飞出/已移除"

### 2.3 为什么 `>=` 而非 `==`

初始设计时，`m_droneCmdIds` 中可能包含一些状态异常的无人机（如 ST_FAULT），它们不会变成 ST_IDLE。如果使用 `==`，这些故障无人机会导致永远无法判定为"完成"。但 `>=` 引入了另一个问题：只要全局 ST_IDLE 数量达标，就会误判。

### 2.4 两个深层矛盾

| 矛盾 | 说明 |
|------|------|
| **计数范围矛盾** | FormationChecker 统计全局 ST_IDLE，但只应统计当前编队的 N 架 |
| **状态归属矛盾** | 飞出无人机用 `ST_IDLE`（绿色）表示"不参与编队"，但 `ST_IDLE` 同时用于"已到达目标"——两者语义冲突 |

---

## 三、解决方案

### 3.1 方案概述

需要同时修改 **FormationChecker** 和 **TestStateReceiver**，形成一个完整的闭环：

```
FormationChecker：只统计"活跃编队成员"的 ST_IDLE
TestStateReceiver：飞出无人机使用 ST_LOST，永不参与判定
```

### 3.2 修改点 1：FormationChecker — 精确计数

**文件**：`core/FormationChecker.h`、`core/FormationChecker.cpp`

**问题**：`countIdleLocked() >= m_expectedCount` 将飞出无人机的 ST_IDLE 也纳入统计。

**方案 A**（推荐）：增加 `m_activeDroneIds` 集合，只统计活跃无人机。

```cpp
// FormationChecker.h 新增成员
std::set<uint32_t> m_activeDroneIds;  // 当前编队的无人机 ID 集合

// FormationChecker.h 新增方法
void setActiveDrones(const std::set<uint32_t>& droneIds);

// FormationChecker.cpp 修改判定逻辑
bool FormationChecker::isFormationCompleteInternal() const {
    if (m_expectedCount == 0) return false;
    if (m_activeDroneIds.size() < m_expectedCount) return false;

    size_t idleCount = 0;
    for (uint32_t droneId : m_activeDroneIds) {
        auto it = m_droneCmdIds.find(droneId);
        if (it == m_droneCmdIds.end() || it->second != m_latestCmdId) {
            return false;  // 有活跃无人机未完成
        }
        auto sit = m_droneStatuses.find(droneId);
        if (sit != m_droneStatuses.end() && sit->second == DroneStatus::ST_IDLE) {
            idleCount++;
        }
    }
    return idleCount >= m_expectedCount;
}
```

**方案 B**（备选，更简单但语义不精确）：将 `>=` 改为 `==`，同时排除 `transitioning` 无人机。

```cpp
bool FormationChecker::isFormationCompleteInternal() const {
    if (m_expectedCount == 0) return false;
    if (m_droneCmdIds.size() < m_expectedCount) return false;

    size_t idleCount = 0;
    for (const auto& [droneId, cmdId] : m_droneCmdIds) {
        if (cmdId != m_latestCmdId) return false;
        auto it = m_droneStatuses.find(droneId);
        if (it != m_droneStatuses.end() && it->second == DroneStatus::ST_IDLE) {
            idleCount++;
        }
    }
    return idleCount == m_expectedCount;  // 精确匹配
}
```

> 方案 B 的前提是：飞出无人机必须在 `m_droneCmdIds` 被清理（`removeDrone`）之前就变为非 ST_IDLE 状态，且 `m_droneCmdIds` 中不包含任何已移除的无人机。

### 3.3 修改点 2：TestStateReceiver — 飞出无人机状态

**文件**：`test/TestStateReceiver.cpp`

**问题 1**：`setFormationTarget` 中重置 `m_reached` 时跳过了 `m_flyOutDrone[i]` 为 true 的无人机。

**修改**：去掉 `!m_flyOutDrone[i]` 条件，改为：

```cpp
// setFormationTarget() 中
for (uint32_t i = 0; i < m_droneCount; ++i) {
    if (!m_removed[i]) {
        m_reached[i] = false;  // 所有非移除无人机都重置，包括飞出的
    }
}
```

**问题 2**：飞出无人机的状态生成逻辑中，`m_reached[i]` 可能为 true（旧值），导致 `ST_IDLE`。

**修改**：在 `onTimerTick` 状态生成中，飞出无人机强制设为 `ST_LOST`（失联），而非依赖 `m_reached`：

```cpp
// onTimerTick() 状态生成部分
if (m_flyOutDrone[i]) {
    // 飞出无人机：强制 ST_LOST，不参与编队判定
    state.status = DroneStatus::ST_LOST;
    state.transitioning = true;
} else if (m_faultDrone[i]) {
    state.status = DroneStatus::ST_FAULT;
} else if (m_landingDrone[i]) {
    state.status = DroneStatus::ST_LANDING;
} else if (m_landedDrone[i]) {
    state.status = DroneStatus::ST_LANDED;
} else if (m_flyInDrone[i]) {
    state.status = DroneStatus::ST_CRUISE;
} else {
    state.status = m_reached[i] ? DroneStatus::ST_IDLE : DroneStatus::ST_CRUISE;
}
```

### 3.4 修改点 3：MainWindow — 同步活跃无人机集合

**文件**：`MainWindow.cpp`

**新增**：在发送队形命令时，将当前活跃的无人机 ID 集合传递给 FormationChecker。

```cpp
// onFormation() 中新增
std::set<uint32_t> activeIds;
for (uint32_t i = 1; i <= N; ++i) {
    activeIds.insert(i);
}
m_formationChecker->setActiveDrones(activeIds);
```

### 3.5 修改点 4：IFormationChecker — 接口扩展

**文件**：`core/IFormationChecker.h`

```cpp
// 新增纯虚方法
virtual void setActiveDrones(const std::set<uint32_t>& droneIds) = 0;
```

### 3.6 完整修改文件清单

| 序号 | 文件 | 修改内容 |
|------|------|----------|
| 1 | `core/IFormationChecker.h` | 新增 `setActiveDrones` 纯虚方法 |
| 2 | `core/FormationChecker.h` | 新增 `m_activeDroneIds` 成员 + `setActiveDrones` 声明 |
| 3 | `core/FormationChecker.cpp` | 修改 `isFormationCompleteInternal` 只统计活跃无人机 |
| 4 | `test/TestStateReceiver.cpp` | 飞出无人机改用 `ST_LOST`；`setFormationTarget` 重置所有非移除无人机的 `m_reached` |
| 5 | `MainWindow.cpp` | `onFormation` 中调用 `setActiveDrones` 传递活跃无人机集合 |

### 3.7 修改后的判定流程

```
用户点击"切换队形"(N=8)
  → MainWindow 组装 activeDrones = {1,2,3,4,5,6,7,8}
  → FormationChecker.setActiveDrones({1,2,3,4,5,6,7,8})
  → FormationChecker.setExpectedCount(8)
  → FormationChecker.setLatestCmdId(cmd_3)

onTimerTick:
  → 无人机 1~8：m_reached=false → ST_CRUISE（黄色）
  → 无人机 9~50：m_flyOutDrone=true → ST_LOST（灰色）
  → callback → FormationChecker 记录所有状态

onCompletionCheck:
  → 遍历 m_activeDroneIds = {1,2,3,4,5,6,7,8}
  → 无人机 9~50 不在 activeDrones 中 → 完全忽略
  → 无人机 1~8 的 cmd_id 检查 + 状态检查
  → 8 架全部 ST_IDLE 且 cmd_id == cmd_3 → 判定完成 ✓
```

---

## 四、状态颜色语义对照

| 状态 | 颜色 | 在当前编队中 | FormationChecker 是否统计 |
|------|------|-------------|--------------------------|
| `ST_IDLE` | 绿色 | 是 | ✓ 统计（已到达目标） |
| `ST_TAKEOFF` | 琥珀 | 是 | ✗ 不统计 |
| `ST_CRUISE` | 黄色 | 是 | ✗ 不统计 |
| `ST_LANDING` | 蓝色 | 是 | ✗ 不统计 |
| `ST_LANDED` | 紫色 | 是 | ✗ 不统计 |
| `ST_FAULT` | 红色 | 是 | ✗ 不统计 |
| `ST_LOST` | 灰色 | 否 | ✗ 不统计 |

> `ST_LOST` 的语义恰好匹配"飞出/不在编队中"的场景：灰色表示失联，FormationChecker 不统计，视觉效果也区别于正常编队无人机。