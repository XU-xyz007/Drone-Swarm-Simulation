# A-DONE

> 组员 A（主控端 + 可视化大屏）完成状态追踪
>
> 版本：v1.10 | 创建：2026-08-31 | 最后更新：2026-09-01（无人机增减动画 + 画面缩放拖拽）

---

## 环境配置信息

| 项目 | 值 |
|------|-----|
| 操作系统 | Windows |
| IDE | Visual Studio + Qt VS Tools |
| Qt 版本 | 6.5.3 |
| 编译器 | MSVC 2019 64-bit |
| C++ 标准 | C++17 |
| CMake 最低版本 | 3.20 |
| Qt 模块 | Core, Gui, Widgets |
| Qt 安装路径 | E:/Qt_installer/6.5.3/msvc2019_64 |
| AUTOMOC/AUTORCC/AUTOUIC | 已开启 |
| 项目根目录 | QtDemo/ |

---

## 总体进度

| 步骤 | 名称 | 状态 | 完成日期 |
|------|------|------|----------|
| 1 | 环境搭建 | ✅ 已完成 | 2026-08-31 |
| 2 | 数据结构定义 | ✅ 已完成 | 2026-08-31 |
| 3 | 抽象接口定义 | ✅ 已完成 | 2026-08-31 |
| 4 | Fake 实现 | ✅ 已完成 | 2026-08-31 |
| 5 | UI 壳子搭建 | ✅ 已完成 | 2026-08-31 |
| 6 | 可视化渲染 | ✅ 已完成 | 2026-08-31 |
| 7 | 队形完成判定 | ✅ 已完成 | 2026-08-31 |
| 8 | 接口文档撰写 | ✅ 已完成 | 2026-08-31 |
| 8.5 | 无人机增减动画 + 画面交互 | ✅ 已完成 | 2026-09-01 |
| 9 | 替换 Real 实现 | 🔒 等待 D 封装库 | — |
| 10 | 联调与验收 | 🔒 等待 C 交付 | — |

> 图例：✅ 已完成 | 🔄 进行中 | ⬜ 未开始 | 🔒 等待外部 | ❌ 已放弃

---

## 一、已完成的接口

### 1.1 内部数据结构（步骤 2）

| 接口 | 文件位置 | 完成日期 |
|------|----------|----------|
| DroneCmdType 枚举 | core/DroneCmd.h | 2026-08-31 |
| DroneCmd 结构体 | core/DroneCmd.h | 2026-08-31 |
| DroneStatus 枚举 | core/DroneState.h | 2026-08-31 |
| DroneState 结构体 | core/DroneState.h | 2026-08-31 | 2026-09-01 新增 transitioning 字段 |

### 1.2 内部抽象接口（步骤 3 + 8.5）

| 接口 | 文件位置 | 完成日期 | 说明 |
|------|----------|----------|------|
| ICommandSender | core/ICommandSender.h | 2026-08-31 | |
| IStateReceiver | core/IStateReceiver.h | 2026-08-31 | 2026-09-01 新增 setOnDroneRemoved 回调 |
| IDroneStateCache | core/IDroneStateCache.h | 2026-08-31 | 2026-09-01 新增 removeState 接口 |
| IFormationChecker | core/IFormationChecker.h | 2026-08-31 | |

### 1.3 内部实现类（步骤 3）

| 类 | 文件位置 | 完成日期 |
|------|----------|----------|
| DroneStateCache | core/DroneStateCache.h/.cpp | 2026-08-31 |
| FormationChecker | core/FormationChecker.h/.cpp | 2026-08-31 |

### 1.4 Fake 实现（步骤 4）

| 类 | 文件位置 | 完成日期 |
|------|----------|----------|
| FakeCommandSender | fake/FakeCommandSender.h/.cpp | 2026-08-31 |
| FakeStateReceiver | fake/FakeStateReceiver.h/.cpp | 2026-08-31 |

### 1.5 UI 组件（步骤 5）

| 组件 | 文件位置 | 完成日期 |
|------|----------|----------|
| MainWindow | MainWindow.h/.cpp | 2026-08-31 |

### 1.6 渲染类（步骤 6）

| 类 | 文件位置 | 完成日期 |
|------|----------|----------|
| ColorConfig | render/ColorConfig.h | 2026-08-31 |
| DroneItem | render/DroneItem.h/.cpp | 2026-08-31 |
| CoordinateMapper | render/CoordinateMapper.h/.cpp | 2026-08-31 | 2026-09-01 排除 transitioning + resetView |

---

## 二、未完成的接口

| 接口 | 优先级 | 阻塞原因 |
|------|--------|----------|
| RealCommandSender | 🔒 | 等待 D 封装库 |
| RealStateReceiver | 🔒 | 等待 D 封装库 |

---

## 三、待定内容

| 项目 | 需要谁确认 | 状态 |
|------|-----------|------|
| Topic 名称 | D | ⚠️ |
| D 封装库函数签名 | D | ⚠️ |
| 无人机速度上限 | 全组 | ⚠️ |
| 到达判定阈值 | 全组 | ⚠️ |
| B 的队形编号表 | B | ⚠️ |
| 可视化画布默认视野 | A（自行决定） | ⚠️ |
| 轨迹保留帧数（默认 20） | A（自行决定） | ⚠️ |
| 无人机点半径（默认 5.0 px） | A（自行决定） | ⚠️ |

---

## 四、文件清单

### 4.1 已创建的文件

| 文件路径 | 步骤 | 完成日期 |
|----------|------|----------|
| QtDemo/CMakeLists.txt | 1 | 2026-08-31 |
| QtDemo/QtDemo/CMakeLists.txt | 1 | 2026-08-31 |
| QtDemo/QtDemo/QtDemo.h | 1 | 2026-08-31 |
| QtDemo/QtDemo/QtDemo.cpp | 1-6 | 2026-08-31 |
| QtDemo/QtDemo/MainWindow.h | 5-6 | 2026-08-31 |
| QtDemo/QtDemo/MainWindow.cpp | 5-6 | 2026-08-31 |
| core/DroneCmd.h | 2 | 2026-08-31 |
| core/DroneState.h | 2 | 2026-08-31 |
| core/ICommandSender.h | 3 | 2026-08-31 |
| core/IStateReceiver.h | 3 | 2026-08-31 |
| core/IDroneStateCache.h | 3 | 2026-08-31 |
| core/IFormationChecker.h | 3 | 2026-08-31 |
| core/DroneStateCache.h | 3 | 2026-08-31 |
| core/DroneStateCache.cpp | 3 | 2026-08-31 |
| core/FormationChecker.h | 3 | 2026-08-31 |
| core/FormationChecker.cpp | 3 | 2026-08-31 |
| fake/FakeCommandSender.h | 4 | 2026-08-31 |
| fake/FakeCommandSender.cpp | 4 | 2026-08-31 |
| fake/FakeStateReceiver.h | 4 | 2026-08-31 |
| fake/FakeStateReceiver.cpp | 4 | 2026-08-31 |
| render/ColorConfig.h | 6 | 2026-08-31 |
| render/DroneItem.h | 6 | 2026-08-31 |
| render/DroneItem.cpp | 6 | 2026-08-31 |
| render/CoordinateMapper.h | 6 | 2026-08-31 |
| render/CoordinateMapper.cpp | 6 | 2026-08-31 |
| docs/interface/06-A界面交互.md | 8 | 2026-08-31 |
| test/TestStateReceiver.h | 测试 | 2026-08-31 |
| test/TestStateReceiver.cpp | 测试 | 2026-08-31 |

### 4.2 计划创建的文件

| 文件路径 | 所属步骤 | 状态 |
|----------|----------|------|
| real/RealCommandSender.h | 步骤 9 | 🔒 |
| real/RealCommandSender.cpp | 步骤 9 | 🔒 |
| real/RealStateReceiver.h | 步骤 9 | 🔒 |
| real/RealStateReceiver.cpp | 步骤 9 | 🔒 |


## 五、变更记录

| 日期 | 变更内容 |
|------|----------|
| 2026-08-31 | 创建文件 |
| 2026-08-31 | 步骤 1：Qt 6.5.3 + MSVC 2019 + CMake 项目可运行 |
| 2026-08-31 | 步骤 2：DroneCmd.h + DroneState.h |
| 2026-08-31 | 步骤 3：4 个抽象接口 + 2 个实现类 |
| 2026-08-31 | 步骤 4：Fake 实现 + 4 按钮 UI 验证 |
| 2026-08-31 | 步骤 5：MainWindow 独立类 + 完整参数面板 + QGraphicsView 画布 |
| 2026-08-31 | 步骤 6：ColorConfig + DroneItem + CoordinateMapper + MainWindow 渲染接入 |
| 2026-08-31 | 步骤 7：队形判定打磨 + 实时进度 + 状态栏反馈 |
| 2026-08-31 | 步骤 8：06-A界面交互.md 接口文档 |
| 2026-08-31 | 测试模式：TestStateReceiver + 确定性队形 + UI 切换 |
| 2026-08-31 | 使用说明文档 + 测试模式初始位置分散 |
| 2026-08-31 | 04_主控台UI交互与判定逻辑.md + 颜色/判定公式适配组长要求 |
| 2026-08-31 | A-对全组公告.md（通知 B/C/D/组长 A 端接口约定） |
| 2026-09-01 | 无人机增减数量动画：新增从左侧飞入，减少从右侧飞出，不改变画布大小 |
| 2026-09-01 | DroneState 新增 transitioning 字段，CoordinateMapper 排除过渡态无人机 |
| 2026-09-01 | IStateReceiver 新增 setOnDroneRemoved 回调，DroneStateCache 新增 removeState |
| 2026-09-01 | 修复减少数量不生效 Bug：m_userCount 分离用户期望与向量实际大小 |
| 2026-09-01 | 修复多次增减后数量不符 Bug：增减比较改用 m_userCount 而非 m_droneCount |
| 2026-09-01 | 修复队形到达需多次点击 Bug：applySeparation 跳过已到达和近目标无人机 |
| 2026-09-01 | 画面交互：鼠标滚轮缩放 + 拖拽平移 + 双击重置视图适配 |
| 2026-09-01 | 修复缩放/拖拽与 autoFit 冲突：autoFit 仅首次 fitInView，后续只扩展 sceneRect |

---

## 六、每日简要记录

| 日期 | 完成内容 |
|------|----------|
| 2026-08-31 | 步骤 1：环境搭建 |
| 2026-08-31 | 步骤 2：数据结构定义 |
| 2026-08-31 | 步骤 3：抽象接口 + 实现类 |
| 2026-08-31 | 步骤 4：Fake 实现 + 按钮验证 |
| 2026-08-31 | 步骤 5：MainWindow 完整 UI + 参数面板 + 画布 + 状态栏 |
| 2026-08-31 | 步骤 6：渲染类 + 画布显示无人机 |
| 2026-08-31 | 步骤 7：队形判定打磨 + 实时进度 + 状态栏反馈 |
| 2026-08-31 | 步骤 8：06-A界面交互.md 接口文档 |
| 2026-08-31 | 测试模式：TestStateReceiver + 确定性队形 + UI 切换 |
| 2026-08-31 | 使用说明文档 + 测试模式初始位置分散 |
| 2026-08-31 | 04_主控台UI交互与判定逻辑.md + 颜色/判定公式适配组长要求 |
| 2026-08-31 | A-对全组公告.md（通知 B/C/D/组长 A 端接口约定） |
| 2026-09-01 | 步骤 8.5：无人机增减飞入/飞出动画 + transitioning 标记 + removeState 缓存移除 |
| 2026-09-01 | Bug 修复：m_userCount 分离解决减少不生效 + 多次增减数量不符 |
| 2026-09-01 | Bug 修复：applySeparation 跳过已到达/近目标无人机，解决队形到达需多次点击 |
| 2026-09-01 | 画面交互：滚轮缩放 + 拖拽平移 + 双击重置 + autoFit 冲突修复 |

---

## 七、分类索引

### 按类型查找

- **数据结构**：core/DroneCmd.h、core/DroneState.h
- **抽象接口**：core/ICommandSender.h、core/IStateReceiver.h、core/IDroneStateCache.h、core/IFormationChecker.h
- **内部实现**：core/DroneStateCache.*、core/FormationChecker.*
- **假数据**：fake/FakeCommandSender.*、fake/FakeStateReceiver.*
- **渲染**：render/ColorConfig.h、render/DroneItem.*、render/CoordinateMapper.*
- **UI**：MainWindow.*、QtDemo.cpp
- **真实现**：real/RealCommandSender.*、real/RealStateReceiver.*
- **文档**：docs/interface/06-A界面交互.md
- **构建**：CMakeLists.txt

### 按依赖关系查找

- **无依赖**：core/DroneCmd.h、core/DroneState.h、CMakeLists.txt
- **依赖数据结构**：core/I*.h、core/DroneStateCache.*、core/FormationChecker.*、fake/*
- **依赖抽象接口**：MainWindow.*
- **依赖 MainWindow**：render/*
- **依赖 D 封装库**：real/*
- **依赖全组**：docs/interface/06-A界面交互.md