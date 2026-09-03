# A-D UI 与 DDS 联调压测

`QtDemo` 增加了可选的真实 DDS 模式：`QtDemo --dds 99`。默认启动仍是 A 原来的 Fake 模式，避免影响其独立开发。

## 压测流程

1. 使用 **MSVC 版本的 Qt** 编译 `qt_master_ui`（必须与 ZR-DDS 的 MSVC 库同一工具链；MinGW Qt 不能链接 ZR-DDS 的 VS 库）。
2. 启动 UI：`QtDemo --dds 99`。控制台应显示 `state subscriber started on Topic_Drone_State`。
3. 在另一终端启动负载无人机：
   ```powershell
   & ".\x64\Debug\Drone\Drone.exe" 99 50 3 30
   ```
4. 等待两秒，再在第三个终端运行命令发送端：
   ```powershell
   & ".\x64\Debug\Controller\Controller.exe" 99
   ```

## 通过标准

- UI 控制台：`[DDS-UI] received ... state samples/s`。
  - 命令前约 **99 samples/s**（99 架静止，1 Hz）。
  - 命令后约 **549 samples/s**（50 架移动 × 10 Hz + 49 架静止 × 1 Hz）。
- UI 控制台：`[UI-Metrics] render=... frames/s | cached drones=99`。
  正常情况下刷新应接近 20 FPS，缓存数量稳定为 99。
- 画布中前 50 架持续移动，其余 49 架基本静止；窗口不卡死、不崩溃。
- 静止无人机 1 Hz 上报不会被判为离线，因为 DDS QoS 没有设置 Deadline。

## UI 与 DDS 的职责边界

- `DdsCommandSender`：将 A 的 `DroneCmd` 映射为 `SharedMessage::DroneCmd`，发布到 `Topic_Drone_Command`。
- `DdsStateReceiver`：订阅 `Topic_Drone_State`，把 DDS 回调安全投递至 Qt 主线程，再交给 A 的状态缓存与渲染。
- 编队目标计算仍属于 B/C；该压测只验证“多机状态流 → 主控 UI”的吞吐和刷新能力。
