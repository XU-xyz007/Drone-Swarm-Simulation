# B 模块：编队槽位算法

本目录是项目中唯一的编队公式来源。A（主控）和 C（无人机端）必须引用这里的源码，不得复制公式另写一套。

## 功能范围

- `formation_id = 0`：方阵；
- `formation_id = 1`：一字；
- `formation_id = 2`：实心三角点阵；
- `formation_id = 3`：圆形；
- `formation_text` 非空：优先生成 UTF-8 文字队形；
- 一次返回恰好 N 个静态目标槽位；
- 不负责 DDS、移动、避障、最优槽位分配和 `drone_id -> index`。

## 公共接口

A/C 只需包含：

```cpp
#include "FormationAlgorithm.h"
```

调用示例：

```cpp
SwarmFormation::FormationRequest request;
request.formation_id = cmd.formation_id;
request.formation_text = cmd.formation_text; // 转为 UTF-8 std::string
request.count = cmd.expected_count;
request.center_x = cmd.target_x;
request.center_y = cmd.target_y;
request.spacing = cmd.spacing;

const auto result = SwarmFormation::GenerateSlots(request);
if (!result.success) {
    std::cerr << SwarmFormation::FormationErrorMessage(result.error) << std::endl;
    return;
}
```

A 遍历 `result.slots` 绘制期望队形。C 将在线 `drone_id` 升序映射为从 0 开始的连续 `index`，再读取 `result.slots[index]`。

## VS2022 接入

在 Controller 和 Drone 项目中分别使用“添加 -> 现有项”，添加以下 `.cpp`：

```text
FormationAlgorithm.cpp
GeometryUtils.cpp
PresetFormations.cpp
TextFormation.cpp
TextRasterizerWin.cpp
HersheyFont.cpp
```

并将 `$(SolutionDir)SharedMessage\Algorithm` 加到“C/C++ -> 常规 -> 附加包含目录”。同时必须随程序部署 `assets/chinese-hershey-font/Heiti.hf.txt`；缺少该资源时中文会回退到 Windows GDI。`TextRasterizerWin.cpp` 已声明所需的 `Gdi32.lib` 和 `User32.lib`。

## 约定

- `spacing` 单位为米，表示相邻槽位的最小统一间距；
- `formation_text` 使用 UTF-8；非空时优先于 `formation_id`；
- `count` 最大为 4096；`formation_text` 最大为 16 个字符且不得超过 DDS 字段的 255 个 UTF-8 字节；
- 基本中文优先使用 `assets/chinese-hershey-font/Heiti.hf.txt` 单线矢量黑体，槽位直接沿折线生成；英文或字库未覆盖字符回退到 Windows GDI 栅格化、粗网格降采样和中心骨架细化；
- 槽位初选后执行多轮均匀松弛，减少笔画局部过密或过疏；输入相同，槽位数量、坐标和顺序必须完全一致；
- 中文单线字库来自 [LingDong-/chinese-hershey-font](https://github.com/LingDong-/chinese-hershey-font)，覆盖 20975 个基本 CJK 汉字，采用 MIT License；许可证副本位于字库目录。
- 方阵最后一行从左向右填充，不单独居中；
- 圆形 `N == 1` 时返回中心点；
- B 只生成目标槽位，C 负责移动和状态发布；
- V1 不承诺飞行过程无穿模。

## 测试

`tests/FormationAlgorithmTests.cpp` 覆盖四种基础队形、`S`、`AI`、`DDS`、`中`、`爱`、`无人机`、非法输入、输入上限、确定性、重复槽位、间距和 `N == 1`。高密度中文还会检查最近邻平均距离与最大/最小比，避免只满足最小安全距离却整体松散。笔画细化最多执行 64 轮，避免异常字体阻塞业务线程。

手动预览会同时生成 `manual-preview.svg`（自动适配）和 `manual-preview-fixed-scale.svg`（固定 `8 px/m`），并显示实际宽高、最近邻均值、标准差及最大/最小比。固定比例尺视图用于发现实际队形尺寸异常，不能只看自动适配图。
