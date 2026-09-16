# 从零开始：跑通并读懂你的第一个 C++ 项目

这份文档假设你**从没写过 C++**，也不需要任何前置知识。目标很具体：让你自己把这个仓库构建起来、看懂里面每一类文件的作用、并且独立地改出一处新功能。

阅读方式：**边读边在终端里敲**。每节末尾有「过关检查」，做不出来就停在那一节，不用急着往下赶。

项目位置：`D:\codex项目\Mini Perception Stack`

---

## 第 0 章 · 先搞清三件事（约 10 分钟）

### 0.1 C++ 不是"直接运行"的语言

你写下的 `.cpp` 文件是**给人看的文本**，电脑看不懂。中间必须有人把它翻译成机器码：

```
你写的 add.cpp  ──[编译器 cl.exe]──>  sensekit-stats.exe  ──[命令行运行]──>  屏幕上的结果
```

翻译这一步叫**编译**（compile），做翻译的程序叫**编译器**（compiler）。这就是为什么改完代码必须重新构建一次，否则你运行的还是上一次生成的旧 exe。

### 0.2 "库"和"程序"是两种东西

- **库（library）**：一堆函数的集合，自己没有入口、不能单独运行。本项目里叫 `sensekit`。
- **程序（可执行文件）**：带 `main()` 函数的那一个，能直接在命令行跑。本项目里叫 `sensekit-stats.exe`。

为什么要拆开？因为库能被多个程序复用，也能被测试程序直接调用。到了阶段二，特征提取、滤波都会继续加进同一个库，而调用它的程序只会越来越多。

### 0.3 CMake 不是编译器

新手最容易混淆的一点：**CMake 不编译代码**，它是"生成编译指令"的工具。

```
CMakeLists.txt  ──[cmake]──>  build/msvc/*.vcxproj  ──[MSBuild 调 cl.exe]──>  .exe
  你写的规则                    自动生成的工程文件        真正干活的
```

所以构建分两步：先 **configure**（配置，生成工程文件），再 **build**（构建，真正编译链接）。配置只需要做一次，之后改代码只要 build。

---

## 第 1 章 · 你机器上的工具链

| 工具 | 干什么的 | 在这台机器上的位置 |
| --- | --- | --- |
| VS Code | 编辑器，写代码的地方 | `D:\Microsoft VS Code` |
| MSVC（`cl.exe`） | 编译器，把 C++ 翻译成 exe | `D:\VS\2022\BuildTools` |
| CMake | 生成构建规则 | 随 VS Build Tools 一起装 |
| ctest | 批量运行测试 | 同上 |
| Git | 版本管理 | `D:\Git` |
| gh | GitHub 命令行 | `D:\dev\gh\bin` |
| Python + numpy | 交叉验证用的第二套实现 | `D:\Python312` |

打开一个**新的** PowerShell 窗口，逐条敲下面的命令，都应该有输出：

```powershell
cmake --version
ctest --version
python --version
git --version
gh --version
```

### 为什么 `cl.exe` 没在 PATH 里？

如果你敲 `cl`，会得到"无法识别"的错误。这是**故意的**，不是装漏了。

`cl.exe` 单独放进 PATH 没用——它运行时还要读一堆环境变量（`INCLUDE`、`LIB` 以及 PATH 里几十个目录），这些变量由 `vcvars64.bat` 设置。传统做法是先跑那个脚本再编译，麻烦且容易出错。

CMake 的 Visual Studio 生成器会**自己去找**编译器并配好这些环境，所以：

> 你只需要会 `cmake --preset msvc`，不需要手动配置编译环境。

`cl.exe` 的真实位置是 `D:\VS\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe`，知道它在哪就够了。

**过关检查：** 上面五条命令都能打印版本号。

---

## 第 2 章 · 五分钟跑通它

在 PowerShell 里进入项目目录，然后依次执行：

```powershell
cd "D:\codex项目\Mini Perception Stack"
cmake --preset msvc
cmake --build --preset msvc-debug
ctest --preset msvc-test-debug
```

三条命令分别是：配置 → 构建 → 测试。第三条最后应该看到：

```
100% tests passed, 0 tests failed out of 55
```

接着运行程序：

```powershell
.\build\msvc\Debug\sensekit-stats.exe .\tests\data\comma_header.csv
```

你会看到：

```
column,count,mean,variance,rms
mean,3,2,0.6666666667,2.160246899
rms,3,20,66.66666667,21.60246899
```

意思是：这个文件有两列，列名分别是 `mean` 和 `rms`；每列 3 个数；`mean` 列的平均值是 2、方差 0.6667、均方根 2.1602……

再故意喂给它一个坏文件：

```powershell
.\build\msvc\Debug\sensekit-stats.exe .\tests\data\ragged.txt
```

```
sensekit-stats: .\tests\data\ragged.txt:2: expected 3 columns but found 2
```

报错格式是 `文件:行号: 出了什么事`。**能给出准确的行号**是这类工具的基本素养，你以后自己写解析器时也要这样。

**过关检查：** 你亲眼看到了那两段输出，退出码分别是 0 和 3（PowerShell 里用 `$LASTEXITCODE` 查看）。

---

## 第 3 章 · 目录结构：每个文件夹为什么存在

```
include/sensekit/     对外公开的头文件
src/                  实现文件（编译成库）
apps/sensekit_stats/  命令行入口（只有一个 main.cpp）
tests/                测试代码 + 测试数据
tools/                辅助脚本（下载数据集、交叉验证）
docs/learning/        学习文档
.github/workflows/    CI 配置
build/                构建产物（被 .gitignore 忽略，不进版本库）
```

### 头文件（.hpp）和源文件（.cpp）的区别

这是 C++ 最反直觉的一点：同一个东西往往要写两遍。

| | 放在哪 | 写什么 |
| --- | --- | --- |
| **声明** | `.hpp` 头文件 | 这个东西**长什么样**：叫什么、返回什么、有几个参数 |
| **定义** | `.cpp` 源文件 | 它**具体怎么做** |

比如 `include/sensekit/stats/stats.hpp` 里写着：

```cpp
[[nodiscard]] double mean(std::span<const double> values);
```

只有一行、以分号结尾，这就是**声明**——告诉编译器"有这么一个函数"。真正的实现（循环求和再除以个数）在 `src/stats/stats.cpp` 里。

为什么要这么麻烦？因为 C++ 是**分开编译**的：每个 `.cpp` 独立编译成目标文件，最后再链接到一起。编译 `stats_cli.cpp` 的时候它看不到 `stats.cpp` 的内容，所以需要头文件提前告诉它函数的形状。

### 为什么 `include/` 下面的路径那么长？

`include/sensekit/stats/stats.hpp` 这种层层嵌套不是装饰，是为了将来用起来清楚：

```cpp
#include "sensekit/stats/stats.hpp"   // 一眼看出来源，也不会重名打架
```

阶段二会加 `sensekit/features/`，阶段三会加 `sensekit/fusion/`，路径本身就是一棵目录树。

**过关检查：** 你能说出 `src/stats/stats.cpp` 和 `include/sensekit/stats/stats.hpp` 各放什么。

---

## 第 4 章 · 逐行读懂 `main.cpp`

文件：`apps/sensekit_stats/main.cpp`。它只有 50 行左右，但包含好几层知识。

```cpp
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "sensekit/cli/stats_cli.hpp"
```

`#include` 的意思是"把另一个文件的内容原样抄进来"。尖括号 `<...>` 表示**标准库或第三方库**（编译器去系统目录找），引号 `"..."` 表示**本项目自己的文件**（去 `include/` 找）。

`#pragma once` 则是"这个头文件在一次编译里最多抄一遍"，防止重复定义。

### 名字空间

```cpp
namespace sensekit::cli {
// ...
}
```

名字空间是**给名字加姓氏**。标准库里有 `sort`，本项目里也可能有，靠名字空间区分。注意 `std::` 前缀不能省：写 `cout` 找不到，必须写 `std::cout`。这一点和 Python 的 `from x import *` 很不一样。

### 真正的入口：Windows 下的 `wmain`

```cpp
#if defined(_WIN32)
int wmain(int argc, wchar_t** argv) {
    ::SetConsoleOutputCP(CP_UTF8);
    // ...
}
#else
int main(int argc, char** argv) {
    // ...
}
#endif
```

`#if defined(_WIN32) ... #else ... #endif` 是**条件编译**：同一份代码在不同平台上编出不同结果。这里的原因很实际：

Windows 给程序传命令行参数用的是 **UTF-16** 编码（每个字符 2 字节）。如果接收时用普通的 `char**`，系统会先按"当前代码页"（中文系统上是 GBK）把参数转成单字节，于是路径里出现"项目"这样的字就会出错。用 `wmain` 拿到原始 UTF-16、再显式转成 UTF-8，就没有歧义了。

这不是理论问题——这个仓库被移动到 `D:\codex项目\` 那天，测试确实因为这个原因全挂了一次。

### `main` 的返回值就是退出码

```cpp
return sensekit::cli::run_stats_cli(args, std::cout, std::cerr);
```

程序最后的返回值会变成**退出码**：

| 退出码 | 含义 |
| --- | --- |
| 0 | 成功 |
| 2 | 参数不对，或文件打不开 |
| 3 | 文件能打开，但内容不是合法的数值表 |

有了这套约定，别人就能在脚本里判断失败原因，比如 `if ($LASTEXITCODE -eq 3) { ... }`。

### 为什么 `main` 里几乎没有逻辑？

它只做三件事：把参数从系统格式转成本项目的格式、调用 `run_stats_cli`、把返回值传出去。**真正的逻辑都在库里**。

这不是为了好看，是为了**可测试**：测试程序可以直接调用 `run_stats_cli({"...", "--ddof", "1"}, out, err)` 并检查输出和退出码，不需要启动一个新进程去跑 exe。你在 `tests/test_stats_cli.cpp` 里看到的端到端测试就是这么写的。

**过关检查：** 用一句话回答："如果我把所有逻辑都写进 `main`、不拆出库，会失去什么？"

---

## 第 5 章 · 数据层：`NumericTable` 与加载器

### "类"是什么

`NumericTable` 表示"一张数值表"：若干行、每行若干列，外加每列的名字。

```cpp
class NumericTable {
public:
    [[nodiscard]] std::size_t row_count() const noexcept { return rows_.size(); }
    [[nodiscard]] std::vector<double> column(std::size_t index) const;
    // ...
private:
    std::vector<std::string> column_names_;
    std::vector<std::vector<double>> rows_;
    std::string source_path_;
};
```

类把**数据**（`column_names_`、`rows_`）和**操作这些数据的函数**绑在一起。`public:` 部分谁都能调，`private:` 部分只有类自己能动——这样外面就不可能把表改成非法状态。

几个语法点：

- `std::vector<std::vector<double>>`：嵌套的二维数组，外层每个元素是"一行"。
- 结尾的下划线 `rows_`：**约定**，表示这是类的内部成员，不是局部变量。
- `[[nodiscard]]`："我返回的值你必须用"。调用了却不接收返回值，编译器会警告。
- `const`（写在函数后面那个）："这个函数不会修改对象内容"。
- `noexcept`："这个函数保证不抛异常"。

### 数据是按行存的

每行数据现在是**连续**存放在内存里的（一个 `vector<double>` 就是一行）。

对阶段二来说这个取舍有好有坏：取整行很便宜，取整列偏贵——`column()` 现在的实现是把每一行的第 N 个元素抄出来，返回一份**拷贝**。

代码注释里写明了这是**有意为之**：阶段一的表很小，先要正确、要简单；阶段二要在 7352×128 的表上反复切窗口，那时拷贝就成了瓶颈，会改成"不拥有数据的列视图"（只指向原表，不复制）。这是真实工程里常见的方式——先简单，等有了真实的性能数字再优化。

### 加载器：把文本变成表

`load_numeric_table()` 负责解析文件，规则六条：

1. 开头的 UTF-8 BOM 自动忽略（记事本存 UTF-8 时会加）；
2. LF 和 CRLF 换行都支持；
3. 空行跳过；
4. 第一个非空字符是 `#` 的行当注释跳过；
5. 每行字段数必须和第一行一致，否则报错并给出**行号**；
6. 字段前后空格自动去掉，所以 ` 1 , 2 ` 和 `1,2` 一样。

分隔符默认 `auto`：首个内容行里出现逗号就按逗号切，否则按连续空格/制表符切。表头也是 `auto`：首个内容行只要有一个字段不是数字，就当成表头；如果全是数字，说明这份文件没有表头，列名自动生成成 `column_0`、`column_1`……

这就是为什么 UCI HAR 那种"空格分隔、没有表头、每行 128 列"的原始信号文件可以直接喂进来，什么都不用配。

### 错误用"异常"表达

```cpp
class LoadError : public std::runtime_error {
public:
    LoadError(LoadErrorKind kind, const std::string& message, std::size_t line_number = 0);
    [[nodiscard]] LoadErrorKind kind() const noexcept { return kind_; }
    // ...
};
```

读取失败时不是返回错误码让调用方检查，而是**抛出异常**：

```cpp
throw LoadError(LoadErrorKind::MalformedRow, "..." + std::to_string(line_number) + ": ...");
```

调用方用 `try { ... } catch (const io::LoadError& error) { ... }` 接住。异常的好处是"错误不可能被悄悄忽略"——一抛，控制流立刻跳到 catch，中间代码不会带着半截数据继续跑。

`LoadError` 里还带了一个 `kind`（错误种类），CLI 用它决定退出码：文件打不开是 2，内容不合法是 3。

**过关检查：** 说出 `--header auto` 在什么情况下会把第一行当表头。

---

## 第 6 章 · 统计层：三个公式，以及它们的边界

三个统计量的定义写得很死，就是为了将来能和 Python 对上：

| 名称 | 公式 |
| --- | --- |
| mean | `sum(x) / N` |
| variance | `sum((x - mean)^2) / (N - ddof)` |
| rms | `sqrt(sum(x^2) / N)` |

`ddof` 是分母的修正量，默认 0（除以 N，叫**总体方差**）；用 `--ddof 1` 就是除以 N-1，叫**样本方差**。为什么默认 0？因为 numpy 的 `np.var(x, ddof=0)` 也是这个口径，两边直接对得上，不用换算。

### 边界情况才是真正的工作量

```cpp
double variance(std::span<const double> values, Ddof ddof) {
    require_not_empty(values);
    const auto divisor = static_cast<double>(values.size()) - static_cast<double>(ddof_divisor(ddof));
    if (divisor <= 0.0) {
        throw std::invalid_argument("sensekit::stats: sample variance needs at least two values");
    }
    // ...
}
```

两种情况不允许：

- **空输入**：一个数都没有，"平均值"没有定义 → 抛异常。
- **样本方差但只有 1 个数**：要除以 `1-1=0`，数学上无意义 → 抛异常。

而**常量序列**（比如 `{5,5,5,5}`）是合法的，方差正好等于 0。它不是错误，是正确答案——区分"合法的 0"和"未定义的运算"，是写数值代码时最基本的一步。

### 为什么遍历两遍？

算方差要先知道均值，所以代码先算均值、再算偏差平方和。也有"一遍扫描"的写法（Welford 之类），更快但更难写对，而且当数值很大又彼此接近时会明显丢精度。

现阶段的表很小，**正确和可读优先**，所以用最直白的两遍写法。等性能真的成为问题再换——这也是整份文档想传递的态度。

**过关检查：** 不翻文档，说出 `{1,2,3,4}` 的 mean、variance（ddof=0）、rms，然后用程序验证。

---

## 第 7 章 · 命令行层：参数、输出、退出码

`src/cli/stats_cli.cpp` 里分三块，边界很清楚：

| 部分 | 负责 |
| --- | --- |
| `parse_options` | 把 `--ddof 1` 这种文本变成结构体里的值 |
| `resolve_columns` | 把 `--columns 0,2` 或 `--columns rms` 变成列下标 |
| `run_stats_cli` | 串起"读文件 → 算统计 → 写输出 → 返回退出码" |

参数支持两种写法，`--ddof 1` 和 `--ddof=1` 等价。解析时先找等号：

```cpp
std::string_view name = argument;
std::string_view inline_value;
bool has_inline_value = false;
if (const auto equals = argument.find('='); equals != std::string::npos) {
    name = std::string_view(argument).substr(0, equals);
    inline_value = std::string_view(argument).substr(equals + 1);
    has_inline_value = true;
}
```

上面这个 `if (初始化; 条件)` 写法（C++17 起）把变量的作用域限制在 if 里面，出了 if 就看不见，避免污染后面的代码。

### 输出格式是有意定死的

表头固定为 `column,count,mean,variance,rms`，数字统一用 `%.10g`（10 位有效数字）打印：

```cpp
buffer << "column,count,mean,variance,rms\n";
buffer << std::setprecision(10);
```

10 位有效数字是个折中：肉眼看着不算太长，又足够精确到能和 numpy 的 double 结果对齐。

### 三种错误用不同退出码分开

- 用了不认识的参数、文件不存在 → 2
- 文件能读但格式不对（列数不齐、非数字、严格模式下出现 NaN） → 3

区分这两种很重要：2 是"你调用方式错了"，3 是"你的数据有问题"，脚本里往往要分别处理。

**过关检查：** 跑 `sensekit-stats --help` 读完帮助文本；再故意传一个不存在的文件，确认退出码是 2。

---

## 第 8 章 · 测试：它们在测什么

`tests/` 下有四个文件：

| 文件 | 测什么 |
| --- | --- |
| `test_stats.cpp` | 纯数学：公式对不对、边界处理对不对 |
| `test_load_numeric_table.cpp` | 解析器：各种格式、各种坏输入 |
| `test_stats_cli.cpp` | 端到端：从参数到输出的完整行为 |
| `data/` | 测试数据与"期望输出"文件 |

### 单元测试 vs 端到端测试

**单元测试**只碰一小块代码，不读文件：

```cpp
TEST(StatsVariance, SampleVarianceMatchesHandComputedValue) {
    // deviations from the mean 2.5 are -1.5, -0.5, 0.5, 1.5 -> sum of squares 5 -> 5/3
    const std::vector<double> values{1.0, 2.0, 3.0, 4.0};
    EXPECT_NEAR(variance(values, Ddof::Sample), 5.0 / 3.0, kTolerance);
}
```

写法是 `TEST(分组名, 用例名)`。`EXPECT_*` 断言失败会继续往下跑，`ASSERT_*` 失败会立刻结束这个用例。注意期望值是**手算后写在注释里**的，这是好习惯：将来有人改公式，能一眼看出改动破坏了什么。

**端到端测试**走完整条链路，还比对**整份输出文件**：

```cpp
TEST(StatsCli, SpaceSeparatedFixtureMatchesTheGoldenCsv) {
    const auto result = run_cli({to_utf8_argument(data_file("space_no_header.txt"))});
    ASSERT_EQ(result.code, kSuccess) << result.err;
    EXPECT_EQ(result.out, read_text(data_file("space_no_header.expected.csv")));
}
```

这就是**黄金文件（golden file）**手法：把正确输出存成 `.expected.csv`，每次跑测试逐字节比对。它比"只检查退出码是不是 0"强得多——格式变了、精度变了、列顺序变了都会被抓住。

代价是改动输出格式时必须同步更新期望文件，而这正好强迫你意识到"我改了对外行为"。

### 只跑一个测试 / 加一个测试

```powershell
# 列出所有测试名（不运行）
ctest --preset msvc-test-debug -N

# 只跑名字里含 Variance 的
.\build\msvc\Debug\tests\sensekit_tests.exe --gtest_filter="*Variance*"

# 只重跑上次失败的
ctest --preset msvc-test-debug --rerun-failed
```

加测试的方法：在对应的 `test_*.cpp` 里写一个新的 `TEST(...)` 块，重新构建，`ctest` 就会自动多出一条。**不需要**手动登记，CMake 会自动发现。

**过关检查：** 打开 `tests/data/ragged.txt`，说出它为什么失败、报错里会提到第几行。

---

## 第 9 章 · 逐段读 `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.25)
project(mini_perception_stack VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
```

前三行是所有 CMake 项目的固定开场：最低版本、项目名、语言、要用的 C++ 标准（这里是 C++20，所以能用上 `std::span`、`std::u8string` 这些新东西）。

### 核心概念：target（目标）

现代 CMake 一切围绕 **target** 展开。这个项目里有三个：

```cmake
add_library(sensekit_warnings INTERFACE)          # 一本"编译选项"说明书
add_library(sensekit ...)                          # 库
add_executable(sensekit-stats apps/.../main.cpp)   # 程序
```

`INTERFACE` 是个特殊类型：它自己不产出任何文件，只**携带设置**，谁链接它谁就拿到这些选项。这样 `/W4`（高警告等级）只作用于我们自己的代码；如果直接用全局的 `add_compile_options`，通过 `FetchContent` 拉进来的 GoogleTest 也会被套上，刷出一屏与己无关的警告。

### `PUBLIC` 和 `PRIVATE` 的区别

```cmake
target_include_directories(sensekit PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")
target_link_libraries(sensekit PRIVATE sensekit_warnings)
```

- `PUBLIC`：我自己要用，**用到我的人也要用**。`include/` 目录就是这种——任何链接 `sensekit` 的目标都必须能找到 `#include "sensekit/..."` 的头文件。
- `PRIVATE`：我自己要用，但**不往外传**。警告选项就属于这种，链接者不需要知道我用什么警告等级。

判断方法：这个设置会不会出现在我的**公开头文件**里？会 → `PUBLIC`；不会 → `PRIVATE`。

### GoogleTest 是怎么来的

`tests/CMakeLists.txt` 里：

```cmake
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(googletest)
```

`FetchContent` 会**在配置阶段自动下载**指定版本的源码并一起构建，所以别人克隆你的仓库不需要手动装 GoogleTest。版本号写死（`v1.15.2`），保证你的机器、CI、同学的机器上跑的测试框架完全一致——这叫可复现构建。

注意：**首次**配置需要联网，之后缓存在 `build/msvc/_deps/` 里，断网也能构建。

### `CMakePresets.json` 是干什么的

它把一长串参数固定成名字：

```json
{ "name": "msvc-release", "configurePreset": "msvc", "configuration": "Release" }
```

于是 `cmake --build --preset msvc-release` 就等于"用 MSVC、x64、Release 配置来构建"。好处是**你不用背参数**，而且 VS Code 的 CMake 扩展能直接认出这些预设。

**过关检查：** 说出为什么 `target_include_directories(... PUBLIC ...)` 不能随便改成 `PRIVATE`。

---

## 第 10 章 · CI：为什么要在别人的机器上再构建一遍

`.github/workflows/ci.yml` 很短：

```yaml
on:
  push:
    branches: [main]

jobs:
  build-and-test:
    runs-on: windows-2022
    steps:
      - uses: actions/checkout@v4
      - run: cmake --preset msvc
      - run: cmake --build --preset msvc-release
      - run: ctest --preset msvc-test-release
```

含义：每次往 `main` 推送，GitHub 就开一台**全新的** Windows 虚拟机，克隆代码，重新走一遍配置 → 构建 → 测试。

### 为什么"我本机能编过"不够

因为你的机器上有太多**你没意识到的东西**：随手装过的软件、改过的环境变量、某个旧缓存、某个只存在于你这里的文件。别人的机器上这些都没有。

CI 就是那台"什么都没有"的机器。它绿了，才说明**仓库里记录的东西是完整的**。这个项目今天就吃过两次亏：

1. 我写了一句 `where.exe cl` 做诊断，本机（装过编译器环境）没问题，CI 上直接失败——干净环境里 `cl` 根本不在 PATH 里。
2. CI 第一次跑就暴露了"`windows-latest` 镜像里已经没有 Visual Studio 2022"这件事，本机完全看不到。

**过关检查：** 打开仓库的 Actions 页面，找到最近一次成功运行的日志，指出 configure / build / test 各在哪几行。

---

## 第 11 章 · 跟做练习

按顺序做。每个练习都有过关标准，**不要跳过验算**。

### 练习 1（10 分钟）：先预测，再验证

在 `tests/data/` 下新建 `mine.txt`，内容自定（比如三行两列）。先**用纸笔算出**每列的 mean / variance / rms，再跑程序对答案。

```powershell
.\build\msvc\Debug\sensekit-stats.exe .\tests\data\mine.txt
```

过关标准：手算结果与程序输出相对误差小于 1%。对不上就找出原因，**别改程序去迁就答案**。

### 练习 2（40 分钟）：给库加一个统计量

加 `min` 和 `max`，或者直接加一个 `peak_to_peak`（峰峰值 = max - min）。

提示：改三个地方——`stats.hpp` 加声明，`stats.cpp` 加实现，`test_stats.cpp` 加测试。先不要动 CLI，让库的部分先正确。

过关标准：新测试通过；空输入的行为有明确定义（抛异常还是返回 0？想清楚再写）；`ctest` 依然全绿。

### 练习 3（60 分钟）：把它接到命令行上

给 CLI 加 `--preview` 参数：打印前 3 行数据（而不是统计值），用来肉眼检查文件读对了没有。

提示：要改 `Options` 结构体、`parse_options`、`run_stats_cli` 的输出部分，以及 `stats_cli_usage()` 的帮助文本。再加一个端到端测试。

过关标准：`sensekit-stats tests/data/comma_header.csv --preview` 输出合乎预期；不带这个参数时行为完全不变（旧测试全绿）。

### 练习 4（30 分钟）：故意制造 bug，看哪个测试报警

把加载器里处理 CRLF 的那几行删掉：

```cpp
if (!line.empty() && line.back() == '\r') {
    line.remove_suffix(1);
}
```

重新构建，跑测试。

过关标准：你能指出哪几个测试变红、为什么只有它们红。然后恢复代码，确认重新全绿。**这一步的价值在于让你信任测试**：它们不是装饰，真的能抓到你。

### 练习 5（30 分钟）：用第二套实现交叉验证

```powershell
python tools\cross_check_numpy.py .\tests\data\mine.txt
```

过关标准：两份输出在 10 位有效数字上一致。不一致时，先怀疑自己哪一边的公式写错了，而不是急着放宽容差。

---

## 第 12 章 · 报错词典

下面这些都是这个项目**真实踩过**的报错，遇到时对着查。

| 报错 | 真正的原因 | 怎么办 |
| --- | --- | --- |
| `could not find any instance of Visual Studio` | 机器上没有匹配的 Visual Studio，或版本与生成器对不上 | 确认 `D:\VS\2022\BuildTools` 还在；CI 上把运行器固定成 `windows-2022` |
| `The current CMakeCache.txt directory ... is different than the directory ... where CMakeCache.txt was created` | 你把项目连同 `build/` 一起搬了家，缓存里存的是绝对路径 | 删掉整个 `build/` 重新配置。构建产物本来就不该跟着搬家 |
| `No CMAKE_CXX_COMPILER could be found` | 编译器探测失败；在受限沙箱里跑 CMake 也会这样 | 换普通终端重试；确认 Build Tools 已安装 |
| `error C7744: 转义序列 "\xBF1" 超出范围` | `"\xBF"` 后面紧跟 `1`，被当成一个十六进制转义吃掉了 | 把字面量拆开写：`"\xEF\xBB\xBF" "1 2\n"` |
| 测试全红，但 exe 手动跑很正常 | 测试数据路径错（编码或绝对路径问题），或者工作目录不对 | 检查路径里的中文；检查测试是从哪个目录启动的 |
| 报错信息里中文路径变成乱码 | 参数是 GBK 字节、输出按 UTF-8 | 本项目已修：`wmain` 转 UTF-8，输出统一 UTF-8 |
| `LINK : fatal error LNK1561: 必须定义入口点` | 没有 `main` / `wmain`，或者名字拼错了 | 检查入口函数签名 |
| `unresolved external symbol` | 声明了函数却没写实现，或实现文件没加进 `CMakeLists.txt` | 检查那个 `.cpp` 是否列在 `add_library` 里 |
| 首次配置卡在下载 GoogleTest | `FetchContent` 要联网拉源码 | 连上网再配置；拉过一次之后会缓存在 `build/msvc/_deps/` |

---

## 第 13 章 · 下一步

读完并做完练习，你应该能独立完成：构建、跑测试、读代码、加一个小功能、看懂 CI 结果。这正好覆盖了阶段一的四个验收项。

接下来：

1. 把 `docs/notes/stage1-retrospective.md` 填完——那是写给你自己的，不是给别人看的。
2. 对着 `docs/learning/ten-step-loop.md` 第 4 步的十个问题自问自答，答不利索的抄进复盘笔记的缺口清单。
3. 然后进入阶段二：滑动窗口 + 时域/频域特征。

---

## 附录 A · 本项目里出现过的 C++ 语法清单

只列这个仓库真实用到的，够你读懂全部代码。

| 语法 | 一句话解释 | 项目里的例子 |
| --- | --- | --- |
| `#include` | 把另一个文件的内容抄进来 | `#include <vector>` |
| `#pragma once` | 头文件只抄一次 | `stats.hpp` 第一行 |
| `namespace` | 给名字加姓氏，避免重名 | `namespace sensekit::stats {` |
| `std::` | 标准库的前缀，不能省 | `std::vector`、`std::string` |
| `auto` | 让编译器推断类型 | `const auto index = table.find_column("mean");` |
| `const` | 不能改 | `const std::string& name` |
| `&`（引用） | 别名，不复制 | `const std::vector<double>& row(std::size_t) const` |
| `std::vector` | 能自动扩容的数组 | `std::vector<double> values;` |
| `std::string` / `std::string_view` | 拥有字符串 / 只是看别人一眼 | 解析时用 `string_view` 避免拷贝 |
| `std::span` | "一段连续内存"的视图 | `mean(std::span<const double> values)` |
| `enum class` | 有作用域的枚举 | `enum class Ddof { Population, Sample };` |
| `struct` vs `class` | 只差默认可见性 | `struct ColumnStats` / `class NumericTable` |
| `[[nodiscard]]` | 返回值必须被使用 | 几乎所有查询函数 |
| `noexcept` | 保证不抛异常 | `row_count() const noexcept` |
| 构造函数 | 对象创建时自动调用 | `NumericTable(std::vector<std::string> names, ...)` |
| 析构函数与 RAII | 对象销毁时自动释放资源 | `std::ifstream` 离开作用域自动关文件 |
| 继承 | 复用并扩展已有类型 | `class LoadError : public std::runtime_error` |
| `throw` / `try` / `catch` | 抛异常与接异常 | 加载器抛，CLI 接 |
| lambda（`[](){ }`） | 现场写的小函数 | 测试里的 `capture_error([]{ ... })` |
| 范围 for | 遍历容器 | `for (const auto value : values)` |
| 带初始化的 `if` | 变量作用域限制在 if 内 | `if (const auto equals = argument.find('='); ...)` |
| `#if defined(...)` | 条件编译，不同平台编不同代码 | `main.cpp` 里的 `_WIN32` |
| `static_cast` | 显式类型转换 | `static_cast<std::size_t>(argc - 1)` |
| `static constexpr` | 编译期常量 | `NumericTable::npos` |

---

## 附录 B · 命令速查

```powershell
# 配置（只需一次，或删过 build/ 之后）
cmake --preset msvc

# 构建
cmake --build --preset msvc-debug        # 调试版，带完整调试信息
cmake --build --preset msvc-release      # 发布版，开了优化

# 测试
ctest --preset msvc-test-debug
ctest --preset msvc-test-release
ctest --preset msvc-test-debug --rerun-failed
ctest --preset msvc-test-debug -N        # 只列测试名

# 运行
.\build\msvc\Release\sensekit-stats.exe <文件> [选项]

# 只跑某一组测试
.\build\msvc\Debug\tests\sensekit_tests.exe --gtest_filter="*Variance*"

# 与 numpy 交叉验证
python tools\cross_check_numpy.py <文件>

# 版本管理
git status
git add -A
git commit -m "说明这次改了什么"
git push

# 看 CI
gh run list
gh run view --log-failed
```
