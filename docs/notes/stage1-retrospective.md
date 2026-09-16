# 阶段一复盘笔记（待你填写）

这份笔记是十步闭环第 8 步的产物。要求只有一条：**用自己的话写，写不出来就是没学会**。
长度 500~800 字，写完再回头看下面的检查清单。

## 标题建议

《我如何用 CMake 把一个 C++ 项目从零搭到 CI 全绿》

## 可以照着回答的问题

1. `CMakeLists.txt` 里的 `add_library` 和 `add_executable` 分别在做什么？如果我把 `sensekit` 库删掉、把所有 `.cpp` 都塞进可执行文件里，会失去什么？
2. `target_include_directories(... PUBLIC ...)` 里的 `PUBLIC` 是什么意思？换成 `PRIVATE` 会怎样？
3. `CMAKE_CXX_STANDARD` 和 `target_compile_features(... cxx_std_20)` 是同一件事的两种写法吗？
4. 为什么 `sensekit-stats` 的 `main()` 里几乎没逻辑，而是调用 `run_stats_cli()`？这样做换来了什么？
5. `NumericTable` 的构造函数、`load_numeric_table()` 抛异常时，已打开的文件是怎么关掉的？谁负责这件事？
6. `--strict` 为什么不是默认行为？如果默认打开，用户的体验会变成什么样？
7. CI 为什么必须在全新的机器上跑一次构建，而不是「我本机能编过就行」？

## 我的笔记

（在这里写）

## 我还答不上来的问题

（把上面任何一条答不利索的问题抄到这里，它们就是下一轮要补的洞）
