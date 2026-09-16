# Mini Perception Stack

一个用 C++ 从零手写的迷你感知栈：把手机 IMU 的原始信号，一路走完「读数据 → 提特征 → 融合估计 → 神经网络推理 → 输出活动标签」的完整链路。

这个仓库同时是一个学习项目：每一阶段的代码、笔记和验收标准都在仓库里，方便对照「我到底学会了没有」。

## 当前进度

**阶段一 · C++ 工程基础与数据 I/O（进行中）**

| 验收项 | 状态 |
| --- | --- |
| CMake 能成功构建 | 见下方「快速开始」 |
| 命令行程序能读数据文件并输出统计值 | `sensekit-stats` |
| `ctest` 全绿 | GoogleTest 单测 + 端到端测试 |
| 仓库有 README 与 CI 配置 | `.github/workflows/ci.yml` |

后续四个阶段（信号处理与特征工程 / 传感器融合与状态估计 / 深度学习与模型部署 / 系统集成与性能优化）只在地图里，暂未实现。完整的十步学习闭环见 [docs/learning/ten-step-loop.md](docs/learning/ten-step-loop.md)。

## 环境要求

- Windows 10/11
- Visual Studio 2022 **Build Tools**，勾选「使用 C++ 的桌面开发」工作负载（提供 MSVC、Windows SDK、CMake、Ninja）
- CMake 3.25 或更新版本
- （可选，做交叉验证时需要）Python 3.10+ 与 numpy
- 首次配置时会联网下载 GoogleTest，之后可离线构建

在 PowerShell 里确认工具链就位：

```powershell
cmake --version
```

`cl.exe` 不需要手动进 PATH —— CMake 的 Visual Studio 生成器会自己找到编译器，这也是本项目没有用 Ninja 生成器的原因：普通终端直接能构建，不必先跑 `vcvars64.bat`。

## 快速开始

```powershell
# 1. 配置（只需一次，之后改代码不用重复）
cmake --preset msvc

# 2. 构建
cmake --build --preset msvc-debug        # 或 cmake --build --preset msvc-release

# 3. 跑测试
ctest --preset msvc-test-debug           # 或 ctest --preset msvc-test-release

# 4. 跑程序
.\build\msvc\Debug\sensekit-stats.exe .\tests\data\comma_header.csv
```

用 VS Code 打开仓库即可：推荐扩展写在 `.vscode/extensions.json`，CMake 预设会被 CMake Tools 自动识别。

## `sensekit-stats` 用法

```
sensekit-stats <file> [options]

options:
  --delimiter auto|comma|whitespace   字段分隔符（默认 auto）
  --header auto|none|first-row        表头处理（默认 auto）
  --columns all|0,2|body_acc_x,...    按 0 基下标或列名选取（默认 all）
  --ddof 0|1                          方差分母取 N 或 N-1（默认 0）
  --output <file>                     结果写入文件而不是 stdout
  --strict                            把 NaN / Inf 当作错误
  -h, --help                          查看帮助
```

输出是 CSV，列为 `column,count,mean,variance,rms`：

```powershell
> .\build\msvc\Debug\sensekit-stats.exe .\tests\data\comma_header.csv
column,count,mean,variance,rms
mean,3,2,0.6666666667,2.160246899
rms,3,20,66.66666667,21.60246899
```

退出码：`0` 成功，`2` 参数或文件错误，`3` 数据格式错误。CI 之外，脚本里也能靠这三个码判断失败类型。

### 数据格式约定

- 空行跳过；第一个非空字符是 `#` 的行按注释跳过；UTF-8 BOM 自动忽略。
- `auto` 分隔符：首个内容行里出现逗号就按逗号切，否则按连续空格/制表符切。
- `auto` 表头：首个内容行只要出现一个非数字字段，就把它当表头，否则整份文件都是数据，列名自动生成成 `column_0`、`column_1`……
- 每行字段数必须一致，否则报错并给出**行号**；非数字字段会给出**行号 + 列号**。
- 不接受 `+1.0` 这种带正号写法（`std::from_chars` 的严格行为），要写就写 `1.0`。

### 统计口径

三个统计量都是按列计算，定义写死如下，阶段二和 numpy 对比时不必再猜：

| 名称 | 公式 |
| --- | --- |
| mean | `sum(x) / N` |
| variance | `sum((x - mean)^2) / (N - ddof)`，默认 `ddof = 0`（总体方差） |
| rms | `sqrt(sum(x^2) / N)` |

默认 `ddof = 0` 是为了和 `numpy.var(values, ddof=0)` 直接对齐。需要样本方差时用 `--ddof 1`。

数值以 `%.10g`（10 位有效数字）打印：读数够短，对比 numpy 又足够精确。

## 用 numpy 做独立交叉验证

```powershell
python tools\cross_check_numpy.py .\tests\data\comma_header.csv
```

输出与 `sensekit-stats` 完全同格式，可以直接 `Compare-Object` 或肉眼比对。两个实现独立写出来之后还能对上，才说明这套统计口径没有理解错。

## 拿到真实数据集

```powershell
powershell -ExecutionPolicy Bypass -File tools\fetch_uci_har.ps1
```

数据会解压到仓库外的 `D:\datasets\UCI-HAR`（约 60 MB，不进版本库）。UCI HAR 的 `train\Inertial Signals\body_acc_x_train.txt` 正好是「空格分隔、无表头、每行 128 列」的原始 IMU 信号，直接就能喂给 `sensekit-stats`：

```powershell
.\build\msvc\Release\sensekit-stats.exe "D:\datasets\UCI-HAR\UCI HAR Dataset\train\Inertial Signals\body_acc_x_train.txt"
```

注意：UCI 的服务器本身很慢，而且**不支持断点续传**，实测过约 9 KB/s，整包要接近两小时。脚本会自动校验下载下来的压缩包是否完整（不完整就重下），中途断掉直接重跑即可。

数据到位之后，一条命令就能做完整的阶段一验收（C++ 与 numpy 交叉验证，含容差判定）：

```powershell
powershell -ExecutionPolicy Bypass -File tools\verify_real_data.ps1
```

## 项目结构

```
include/sensekit/     公开头文件（io / stats / cli）
src/                  实现
apps/sensekit_stats/  命令行入口，只有转参和返回值
tests/                GoogleTest 单测、端到端测试与测试数据
tools/                数据集获取与 numpy 交叉验证脚本
docs/learning/        十步学习闭环文档（领域地图、目标、任务表）
docs/notes/           阶段复盘笔记
```

数据流很短，但这条链路后面会长成完整的感知栈：

```
文本文件 ──► NumericTable ──► stats ──► CSV
              (io)             (特征/统计)
                 │
                 └── 阶段二起：滑动窗口 → 时域/频域特征 → 阶段三滤波融合 → 阶段四 ONNX 推理
```

## 常见问题

**`cmake --preset msvc` 报错说找不到生成器**
没装 Visual Studio 2022 的 C++ 工作负载。用 Build Tools 安装器勾上「使用 C++ 的桌面开发」即可。

**第一次配置卡在下载 GoogleTest**
`FetchContent` 要从 GitHub 拉取。网络受限时先连上网络再配置；已经拉过一次之后会缓存在 `build/msvc/_deps`。

**读自己的 CSV 报 `column 3 is not a number`**
多半是文件里混了空列（`,,`）或者某一列是非数字的标签列。先用 `--columns` 只选数值列，或者用 `--header` 明确表头位置。

**为什么 `sensekit-stats.exe` 打印的方差和 Excel 不一样**
Excel 的 `VAR` 是样本方差（`ddof=1`），这里是总体方差。加 `--ddof 1` 就能对上。

## 许可

个人学习项目，未附许可证。
