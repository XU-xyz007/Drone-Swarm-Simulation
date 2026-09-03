# A-接口定义

> 组员 A（主控端 + 可视化大屏）全部接口定义
>
> 版本：v1.1 | 基于 IDL v1.4 冻结 | 最后更新：2026-09-03

---

## 一、消息结构体（基于 IDL v1.4，不可私自改动）

### 1.1 DroneCmdType 枚举 —— 命令类型

```cpp
enum class DroneCmdType : uint32_t {
    CMD_TAKEOFF    = 0,  // 起飞
    CMD_FORMATION  = 1,  // 变队形（广播意图：队形+数量+间距+中心）
    CMD_LAND       = 2,  // 降落（兜底）
    CMD_EMERGENCY  = 3   // 急停（兜底）
};
```

### 1.2 DroneCmd 结构体 —— 主控发出的命令

```cpp
struct DroneCmd {
    uint32_t     cmd_id;          // 命令单号：主控自增（1, 2, 3...），防重发 + 回执
    DroneCmdType cmd_type;        // 命令类型
    uint32_t     formation_id;    // 预设队形编号：0=方阵 / 1=一字 / 2=三角 / 3=圆……
    std::string  formation_text;  // 自定义文字队形，≤16 字符（汉字/英文），空字符串 = 用 formation_id
    double       target_x;        // 队形中心 X 坐标（米）
    double       target_y;        // 队形中心 Y 坐标（米）
    double       spacing;         // 机间间距（米）
    uint32_t     expected_count;  // 无人机数量 N
};
```

| 字段 | 类型 | 说明 | 按钮点击时的填充规则 |
|------|------|------|----------------------|
| `cmd_id` | `uint32_t` | 自增命令号 | 主控本地计数器 +1 |
| `cmd_type` | `DroneCmdType` | 命令类型 | 根据按钮选对应枚举值 |
| `formation_id` | `uint32_t` | 预设队形编号 | 从下拉框获取；非队形命令填 0 |
| `formation_text` | `string` | 文字队形 | 从输入框获取；留空传 `""` |
| `target_x` | `double` | 队形中心 X | 从输入框获取 |
| `target_y` | `double` | 队形中心 Y | 从输入框获取 |
| `spacing` | `double` | 机间间距 | 从输入框获取 |
| `expected_count` | `uint32_t` | 无人机数量 | 从 N 输入框获取 |

### 1.3 DroneStatus 枚举 —— 无人机状态

```cpp
enum class DroneStatus : uint32_t {
    ST_IDLE     = 0,  // 待命：还没起飞，停在地面/初始位置
    ST_TAKEOFF  = 1,  // 起飞中：从地面升起来那一小段
    ST_CRUISE   = 2,  // 巡航：在空中——飞向目标或已就位保持队形
    ST_LANDING  = 3,  // 降落中：正在降落
    ST_LANDED   = 4,  // 已落地：已经降落
    ST_FAULT    = 5,  // 故障：出故障了
    ST_LOST     = 6   // 失联：V1 只定义不实现，预留
};
```

### 1.4 DroneState 结构体 —— 无人机回传的状态

```cpp
struct DroneState {
    uint32_t     drone_id;        // @key —— 无人机身份证（DDS 按此分实例）
    DroneStatus  status;          // 当前状态（画面按此着色）
    double       x;               // 当前 X 坐标（米）
    double       y;               // 当前 Y 坐标（米）
    uint32_t     current_cmd_id;  // 正在执行哪条命令（回执用）
    bool         transitioning;   // 是否处于状态切换中（动画过渡用）
};
```

| 字段 | 类型 | 说明 | 用途 |
|------|------|------|------|
| `drone_id` | `uint32_t` | @key，无人机身份证 | 区分不同飞机，保证多架状态互不覆盖 |
| `status` | `DroneStatus` | 当前状态（7 种） | 决定画面中该点的颜色 |
| `x` | `double` | 当前 X 坐标（米） | 决定画面中该点的横坐标 |
| `y` | `double` | 当前 Y 坐标（米） | 决定画面中该点的纵坐标 |
| `current_cmd_id` | `uint32_t` | 正在执行的命令号 | 用于"队形完成"判定 |
| `transitioning` | `bool` | 是否处于状态切换中 | 动画过渡标志，默认 false |

---

## 二、通信频道配置

| 属性 | 命令频道 | 状态频道 |
|------|----------|----------|
| **方向** | 主控 → 无人机 | 无人机 → 所有人（含主控） |
| **消息类型** | `DroneCmd` | `DroneState` |
| **发送频率** | 按需（点按钮才发一次） | 10Hz（每 100ms 一帧） |
| **QoS** | RELIABLE + KEEP_LAST(10) | BEST_EFFORT |
| **@key** | 无 | `drone_id` |
| **Topic 名称** | 待 D 确定（如 `DroneCmdTopic`） | 待 D 确定（如 `DroneStateTopic`） |

---

## 三、A 内部抽象接口（面向接口编程，解耦外部依赖）

> **设计原则**：主窗口代码不直接依赖 DDS 或任何人的代码，只依赖以下抽象接口。
> 开发阶段用 Fake 实现，联调阶段替换为 D 的 Real 实现，业务代码一行不动。

### 3.1 ICommandSender —— 命令发送抽象接口

```cpp
class ICommandSender {
public:
    virtual ~ICommandSender() = default;

    // 发送一条 DroneCmd 命令
    // 调用时机：用户点击按钮时
    // 返回值：true=发送成功，false=发送失败
    virtual bool sendCommand(const DroneCmd& cmd) = 0;

    // 获取当前最新的 cmd_id
    virtual uint32_t getLastCmdId() const = 0;
};
```

| 方法 | 参数 | 返回值 | 说明 |
|------|------|--------|------|
| `sendCommand` | `const DroneCmd& cmd` | `bool` | 发送命令，成功返回 true |
| `getLastCmdId` | 无 | `uint32_t` | 获取最近一次发送的 cmd_id |

**Fake 实现行为**：`sendCommand` 打印日志 + 自动递增 cmd_id；`getLastCmdId` 返回当前自增值。

**Real 实现行为**（D 交付后替换）：`sendCommand` 调用 D 封装库的 DDS 发送接口；`getLastCmdId` 同。

### 3.2 IStateReceiver —— 状态接收抽象接口

```cpp
class IStateReceiver {
public:
    virtual ~IStateReceiver() = default;

    // 注册状态回调函数
    // 每当收到一架无人机的 DroneState 时触发
    // callback 参数：const DroneState& —— 收到的状态消息
    virtual void setOnStateReceived(
        std::function<void(const DroneState&)> callback) = 0;

    // 注册无人机移除回调（当无人机从列表中移除时触发）
    virtual void setOnDroneRemoved(
        std::function<void(uint32_t droneId)> callback) = 0;

    // 启动接收（开始监听 DDS 状态频道）
    virtual void start() = 0;

    // 停止接收
    virtual void stop() = 0;

    // 设置最新的命令 ID（用于压力测试帧同步）
    virtual void setLatestCmdId(uint32_t cmdId) = 0;

    // 设置无人机数量
    virtual void setDroneCount(uint32_t count) = 0;
};
```

| 方法 | 参数 | 返回值 | 说明 |
|------|------|--------|------|
| `setOnStateReceived` | `std::function<void(const DroneState&)>` | `void` | 注册回调，每次收到状态时触发 |
| `setOnDroneRemoved` | `std::function<void(uint32_t)>` | `void` | 注册回调，无人机被移除时触发 |
| `start` | 无 | `void` | 开始监听状态频道 |
| `stop` | 无 | `void` | 停止监听 |
| `setLatestCmdId` | `uint32_t cmdId` | `void` | 同步最新命令 ID |
| `setDroneCount` | `uint32_t count` | `void` | 设置无人机数量 |

**Fake 实现行为**：`start` 启动一个 Qt 定时器，每 100ms 生成 N 个假 `DroneState`（坐标随机游走、状态循环切换），调用 callback。

**Real 实现行为**（D 交付后替换）：`start` 调用 D 封装库注册 DDS 订阅回调，收到消息后调用 callback。

### 3.3 IDroneStateCache —— 无人机状态缓存接口

```cpp
class IDroneStateCache {
public:
    virtual ~IDroneStateCache() = default;

    // 更新/插入一架无人机的状态
    virtual void updateState(const DroneState& state) = 0;

    // 获取指定无人机的状态（不存在返回 std::nullopt）
    virtual std::optional<DroneState> getState(uint32_t droneId) const = 0;

    // 获取所有已知无人机的状态列表
    virtual std::vector<DroneState> getAllStates() const = 0;

    // 获取当前已知的无人机数量
    virtual size_t getDroneCount() const = 0;

    // 清空所有缓存
    virtual void clear() = 0;
};
```

| 方法 | 参数 | 返回值 | 说明 |
|------|------|--------|------|
| `updateState` | `const DroneState&` | `void` | 更新/插入状态缓存 |
| `getState` | `uint32_t droneId` | `std::optional<DroneState>` | 查询单架状态 |
| `getAllStates` | 无 | `std::vector<DroneState>` | 获取全部状态列表 |
| `getDroneCount` | 无 | `size_t` | 当前已知无人机数量 |
| `clear` | 无 | `void` | 清空缓存 |

**实现**：`DroneStateCache`，内部用 `std::unordered_map<uint32_t, DroneState>` 存储。

### 3.4 IFormationChecker —— 队形完成判定接口

```cpp
class IFormationChecker {
public:
    virtual ~IFormationChecker() = default;

    // 设置预期的无人机总数 N
    virtual void setExpectedCount(uint32_t count) = 0;

    // 设置最新的命令 ID
    virtual void setLatestCmdId(uint32_t cmdId) = 0;

    // 更新一架无人机的 current_cmd_id（由状态回调触发）
    virtual void updateDroneCmdId(uint32_t droneId, uint32_t currentCmdId) = 0;

    // 更新一架无人机的状态（由状态回调触发，用于统计就位数量）
    virtual void updateDroneStatus(uint32_t droneId, DroneStatus status) = 0;

    // 检查是否队形完成（全部 N 架无人机都回执了最新 cmd_id）
    virtual bool isFormationComplete() const = 0;

    // 获取完成进度（已完成数 / 总数）
    virtual std::pair<size_t, size_t> getProgress() const = 0;

    // 重置
    virtual void reset() = 0;
};
```

| 方法 | 参数 | 返回值 | 说明 |
|------|------|--------|------|
| `setExpectedCount` | `uint32_t count` | `void` | 设置预期无人机数量 N |
| `setLatestCmdId` | `uint32_t cmdId` | `void` | 每次发命令后更新 |
| `updateDroneCmdId` | `uint32_t droneId, uint32_t currentCmdId` | `void` | 收到状态时更新 |
| `updateDroneStatus` | `uint32_t droneId, DroneStatus status` | `void` | 收到状态时更新（统计就位） |
| `isFormationComplete` | 无 | `bool` | 全部完成返回 true |
| `getProgress` | 无 | `pair<size_t,size_t>` | (已完成, 总数) |
| `reset` | 无 | `void` | 重置检查器 |

---

## 四、渲染接口

### 4.1 颜色映射表 —— Status → Color

```cpp
// 7 种状态 → 7 种颜色
struct ColorConfig {
    static constexpr QColor ST_IDLE_COLOR    = QColor(76, 175, 80);   // 绿色（待命）
    static constexpr QColor ST_TAKEOFF_COLOR = QColor(33, 150, 243);  // 蓝色（起飞中）
    static constexpr QColor ST_CRUISE_COLOR  = QColor(255, 235, 59);  // 黄色（巡航中）
    static constexpr QColor ST_LANDING_COLOR = QColor(255, 152, 0);   // 橙色（降落中）
    static constexpr QColor ST_LANDED_COLOR  = QColor(158, 158, 158); // 灰色（已落地）
    static constexpr QColor ST_FAULT_COLOR   = QColor(244, 67, 54);   // 红色（故障）
    static constexpr QColor ST_LOST_COLOR    = QColor(96, 125, 139);  // 蓝灰（失联）

    static QColor getColor(DroneStatus status) {
        switch (status) {
            case DroneStatus::ST_IDLE:    return ST_IDLE_COLOR;
            case DroneStatus::ST_TAKEOFF: return ST_TAKEOFF_COLOR;
            case DroneStatus::ST_CRUISE:  return ST_CRUISE_COLOR;
            case DroneStatus::ST_LANDING: return ST_LANDING_COLOR;
            case DroneStatus::ST_LANDED:  return ST_LANDED_COLOR;
            case DroneStatus::ST_FAULT:   return ST_FAULT_COLOR;
            case DroneStatus::ST_LOST:    return ST_LOST_COLOR;
        }
        return ST_IDLE_COLOR;  // 默认灰色
    }
};
```

### 4.2 DroneItem 类 —— 单个无人机图形项

```cpp
class DroneItem : public QGraphicsEllipseItem {
public:
    DroneItem(uint32_t droneId, QGraphicsItem* parent = nullptr);

    // 更新位置（增量重绘核心：只重绘旧位置+新位置）
    void updatePosition(double x, double y);

    // 更新状态颜色
    void updateStatus(DroneStatus status);

    // 获取无人机 ID
    uint32_t getDroneId() const;

    // 添加轨迹点（可选）
    void addTrailPoint(double x, double y);

private:
    uint32_t m_droneId;
    DroneStatus m_status;
    double m_radius = 5.0;  // 无人机点半径（像素）
    std::deque<QPointF> m_trail;  // 轨迹历史（可选）
    QGraphicsPathItem* m_trailItem = nullptr;  // 轨迹线（可选）
};
```

### 4.3 坐标映射 —— 世界坐标 → 画面坐标

```cpp
class CoordinateMapper {
public:
    // 设置世界坐标系范围（米）
    void setWorldRect(double minX, double minY, double maxX, double maxY);

    // 世界坐标 → 画面像素坐标
    QPointF worldToScene(double worldX, double worldY) const;

    // 自动缩放：根据所有无人机位置自动调整视野
    void autoFit(const std::vector<DroneState>& states, double margin = 10.0);

private:
    double m_scaleX = 1.0;   // 像素/米
    double m_scaleY = 1.0;
    double m_offsetX = 0.0;
    double m_offsetY = 0.0;
};
```

---

## 五、UI 控件接口

### 5.1 输入控件与内部变量的绑定

| UI 控件 | 类型 | 绑定的内部变量 | 默认值 |
|---------|------|---------------|--------|
| 无人机数量 N | `QSpinBox` | `m_droneCount` | 8 |
| 队形选择 | `QComboBox` | `m_formationId` | 0（方阵） |
| 自定义文字 | `QLineEdit` | `m_formationText` | `""` |
| 队形中心 X | `QDoubleSpinBox` | `m_targetX` | 0.0 |
| 队形中心 Y | `QDoubleSpinBox` | `m_targetY` | 0.0 |
| 机间间距 | `QDoubleSpinBox` | `m_spacing` | 5.0 |

### 5.2 按钮与槽函数

| 按钮 | Qt 对象名 | 槽函数 | 触发的命令 |
|------|----------|--------|-----------|
| 起飞 | `btnTakeoff` | `onTakeoffClicked()` | 组装 `CMD_TAKEOFF` → 调用 `m_sender->sendCommand()` |
| 切换队形 | `btnFormation` | `onFormationClicked()` | 组装 `CMD_FORMATION` → 调用 `m_sender->sendCommand()` |
| 降落 | `btnLand` | `onLandClicked()` | 组装 `CMD_LAND` → 调用 `m_sender->sendCommand()` |
| 急停 | `btnEmergency` | `onEmergencyClicked()` | 组装 `CMD_EMERGENCY` → 调用 `m_sender->sendCommand()` |

### 5.3 状态栏信息

| 显示项 | 更新时机 | 内容 |
|--------|----------|------|
| 当前 cmd_id | 每次发送命令后 | `"当前命令: #5"` |
| 队形完成进度 | 每次收到状态后 | `"队形进度: 6/8"` |
| 队形完成提示 | `isFormationComplete() == true` | `"✅ 队形完成！"`（绿色） |
| 在线无人机数 | 每次收到状态后 | `"在线: 8 架"` |

---

## 六、外部依赖接口（由其他组员提供，A 只调用不实现）

### 6.1 D 的 DDS 封装库接口（待 D 交付后确认）

```cpp
// 预期接口（D 的接口文档 02 会定义，此处为 A 的预期）
namespace DdsWrapper {
    // 初始化 DDS（传入 Topic 名称等配置）
    bool init(const DdsConfig& config);

    // 发送 DroneCmd
    bool publishCmd(const DroneCmd& cmd);

    // 订阅 DroneState，注册回调
    void subscribeState(std::function<void(const DroneState&)> callback);

    // 关闭 DDS
    void shutdown();
}
```

### 6.2 B 的队形编号表（待 B 交付后确认）

```cpp
// 预期接口（B 的接口文档 03 会定义，此处为 A 的预期）
namespace FormationTable {
    // 队形编号 → 名称映射
    const std::map<uint32_t, std::string> FORMATION_NAMES = {
        {0, "方阵"},
        {1, "一字"},
        {2, "三角"},
        {3, "圆形"},
        // B 可能继续添加……
    };

    // 获取队形名称列表（用于填充下拉框）
    std::vector<std::pair<uint32_t, std::string>> getFormationList();
}
```

### 6.3 全局常量（接口文档 05 定义，A 必须遵守）

| 常量 | 值 | 说明 |
|------|-----|------|
| 坐标系单位 | 米 | 所有坐标使用米制 |
| 状态推送频率 | 10 Hz | 每 100ms 一帧 |
| 状态 QoS | BEST_EFFORT | 丢一帧听下一帧 |
| 命令 QoS | RELIABLE | 丢了自动重发 |
| 速度上限 | 待定（接口 05） | 无人机最大移动速度 |
| 间距默认值 | 5.0 米 | 机间间距默认值 |
| 到达判定阈值 | 待定（接口 05） | 判断无人机到达目标的距离阈值 |
| 失联阈值 | 3 秒 + 2 秒确认延迟 | V1 只写死，不实现 |
| 文字队形最大长度 | 16 字符 | 汉字/英文均≤16 |
| 无人机 ID 范围 | 1 ~ N | 从 1 开始连续编号 |

---

## 七、队形完成判定流程（伪代码）

```
用户点击"切换队形"
    → 组装 DroneCmd（cmd_id = ++m_cmdCounter, cmd_type = CMD_FORMATION, ...）
    → m_sender->sendCommand(cmd)
    → m_formationChecker->setExpectedCount(N)
    → m_formationChecker->setLatestCmdId(cmd.cmd_id)
    → m_formationChecker->reset()
    → 状态栏显示 "队形进度: 0/N"

每收到一个 DroneState:
    → m_stateCache->updateState(state)
    → m_formationChecker->updateDroneCmdId(state.drone_id, state.current_cmd_id)
    → 更新对应的 DroneItem 的位置和颜色
    → 状态栏更新 "队形进度: done/N"

    if m_formationChecker->isFormationComplete():
        → 状态栏显示 "✅ 队形完成！"
        → 可选：弹窗提示
```

---

## 八、文件结构（预期）

```
A_MainControl/
├── CMakeLists.txt                    // 构建配置
├── main.cpp                          // 入口
├── MainWindow.h / .cpp               // 主窗口（UI 布局 + 信号槽连接）
├── core/
│   ├── DroneCmd.h                    // DroneCmdType 枚举 + DroneCmd 结构体
│   ├── DroneState.h                  // DroneStatus 枚举 + DroneState 结构体
│   ├── ICommandSender.h              // 命令发送抽象接口
│   ├── IStateReceiver.h              // 状态接收抽象接口
│   ├── IDroneStateCache.h            // 状态缓存抽象接口
│   ├── IFormationChecker.h           // 队形完成判定抽象接口
│   ├── DroneStateCache.h / .cpp      // 状态缓存实现
│   └── FormationChecker.h / .cpp     // 队形完成判定实现
├── fake/
│   ├── FakeCommandSender.h / .cpp    // 假命令发送器（打印日志 + 自增 cmd_id）
│   └── FakeStateReceiver.h / .cpp    // 假状态接收器（定时器生成随机 DroneState）
├── render/
│   ├── DroneItem.h / .cpp            // 单个无人机图形项
│   ├── ColorConfig.h                 // 颜色映射表
│   └── CoordinateMapper.h / .cpp     // 坐标映射器
├── test/
│   ├── TestStateReceiver.h / .cpp     // 测试模式：固定队形状态生成器
│   ├── StressTestReceiver.h / .cpp    // 压力测试：大规模无人机状态生成器
│   └── TestPresets.h                  // 压力测试预设参数
└── real/
    ├── RealCommandSender.h / .cpp    // D 封装库命令发送器（待 D 交付后实现）
    └── RealStateReceiver.h / .cpp    // D 封装库状态接收器（待 D 交付后实现）
```

---

## 九、接口状态总览

| 接口 | 类型 | 状态 | 依赖 |
|------|------|------|------|
| `DroneCmdType` 枚举 | 数据结构 | ✅ 已冻结（IDL v1.4） | 无 |
| `DroneCmd` 结构体 | 数据结构 | ✅ 已冻结（IDL v1.4） | 无 |
| `DroneStatus` 枚举 | 数据结构 | ✅ 已冻结（IDL v1.4） | 无 |
| `DroneState` 结构体 | 数据结构 | ✅ 已冻结（IDL v1.4） | 无 |
| `ICommandSender` | 抽象接口 | ✅ 已定义 | 无 |
| `IStateReceiver` | 抽象接口 | ✅ 已定义 | 无 |
| `IDroneStateCache` | 抽象接口 | ✅ 已定义 | 无 |
| `IFormationChecker` | 抽象接口 | ✅ 已定义 | 无 |
| `ColorConfig` | 工具类 | ✅ 已定义 | 无 |
| `DroneItem` | 渲染类 | ✅ 已定义 | Qt |
| `CoordinateMapper` | 工具类 | ✅ 已定义 | 无 |
| `FakeCommandSender` | 假实现 | ✅ 已实现 | 无 |
| `FakeStateReceiver` | 假实现 | ✅ 已实现 | Qt |
| `TestStateReceiver` | 测试实现 | ✅ 已实现 | Qt |
| `StressTestReceiver` | 压力测试实现 | ✅ 已实现 | Qt |
| `RealCommandSender` | 真实现 | 🔒 等待 D 封装库 | D 的 DDS 封装库 |
| `RealStateReceiver` | 真实现 | 🔒 等待 D 封装库 | D 的 DDS 封装库 |
| `FormationTable`（B 提供） | 外部依赖 | 🔒 等待 B 交付 | B 的队形算法库 |
| 全局常量（接口 05） | 外部依赖 | ⚠️ 部分待定 | 全组确认 |
| Topic 名称 | 外部依赖 | ⚠️ 待 D 确定 | D 的 DDS 配置 |

> 图例：✅ 已确定 | ⏳ 待实现（A 自己） | 🔒 等待外部交付 | ⚠️ 待确认