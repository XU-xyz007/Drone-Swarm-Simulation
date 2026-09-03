# DDS 状态压测说明（D 提供给 A）

`Drone` 项目现在是一个可配置的 DDS 状态数据生产器。它不替代 C 的真实无人机运动算法；目的是为 A 的 Qt 主控提供大量、可重复、可观察的状态流。

## 启动参数

```text
Drone.exe [无人机总数] [收到命令后移动的数量] [速度(m/s)] [持续秒数]
```

示例：

```powershell
# 99 架；收到主控命令后，前 50 架以 3m/s、10Hz 运动；其余 49 架静止、1Hz 上报；运行 60 秒。
& ".\x64\Debug\Drone\Drone.exe" 99 50 3 60

# 另开终端：下发一次命令（用于触发移动节点）。
& ".\x64\Debug\Controller\Controller.exe" 99
```

在未收到命令前，全部节点为 `ST_IDLE` 且 1Hz 上报；收到新 `cmd_id` 后，指定的移动节点为 `ST_CRUISE`、坐标持续变化且 10Hz 上报。重复收到同一个 `cmd_id` 会被忽略，避免主控的可靠传输重发反复触发状态切换。

## 供 A 验证的场景

| 场景 | 启动参数 | 验收重点 |
|---|---|---|
| 基础 | `5 5 2 30` | 5 个节点平滑移动、命令编号正确 |
| 中等负载 | `50 25 3 60` | 25 个节点 10Hz、25 个节点 1Hz，画面不卡顿 |
| 最大负载 | `99 50 3 60` | 主控完整缓存 99 架，20Hz 渲染保持可操作 |
| 快速运动 | `99 99 8 30` | 99 架全部 10Hz，坐标连续、无闪退 |

## 状态字段

发布 Topic 为 `Topic_Drone_State`，IDL 字段保持不变：

- `drone_id`: 1 到 N 的稳定 DDS 身份；
- `status`: 静止为 `ST_IDLE`，运动为 `ST_CRUISE`；
- `x`, `y`: 米制坐标，边界内反弹；
- `current_cmd_id`: 触发本轮移动的命令编号。

## A 的接入点

A 目前的 `FakeStateReceiver` 已遵守 `IStateReceiver`。联调时只需新建 DDS 版接收器：在 `SwarmComm::SubscribeState` 回调中将 IDL `SharedMessage::DroneState` 映射为 A 的 `DroneState`，再调用 `IStateReceiver` 的 `setOnStateReceived` 所登记的回调。不要把 1Hz 的静止节点当作失联。
