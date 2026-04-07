# 体验服BOT注入器 Design

## Overview

`体验服BOT注入器` 是一个使用 C++20 从零实现的 Windows 原生 BOT 注入工具，用于替代历史上的 `BAT + PowerShell` 脚本方案。

程序目标是：

1. 选择一个 `.zip` 或 `.rar` BOT 注入包
2. 自动或手动定位 `英雄联盟体验服` 根目录
3. 仅提取归档中的 `league of legends/` 子树到目标 `Game` 目录
4. 对本次实际写入成功的文件做后处理
5. 保存最近一次成功路径到 `config.ini:last_path`
6. 输出 UTF-8 日志，便于排障和发布后支持

该实现不再依赖：

- BAT 自提取逻辑
- PowerShell
- WinForms
- ANSI / 默认代码页推断

## Goals

本工程的重写目标是：

1. 彻底摆脱脚本实现负担
2. 保留原脚本的核心业务功能和用户交互语义
3. 增强编码稳定性，统一中文路径与日志处理
4. 增加 `.rar` 支持
5. 建立可维护、可扩展的标准 C++ 工程结构

## Non-Goals

当前版本不追求以下内容：

1. 完整 GUI 应用
2. 多语言界面
3. 通用压缩包管理器能力
4. 对任意游戏或任意归档目录规则的泛化支持

## Functional Behavior

### Archive Input

程序支持两种归档输入方式：

1. 命令行第一个参数作为初始归档路径
2. 若未传入参数，则通过原生文件选择对话框选择归档文件

当前支持的格式：

- `.zip`
- `.rar`

### Target Resolution

目标目录解析逻辑保留了原脚本的业务语义。

合法目标目录定义：

```text
<TargetRoot>
└─ Game
```

其中：

- `<TargetRoot>` 语义上为 `英雄联盟体验服` 根目录
- 若用户手动选择 `...\英雄联盟体验服\Game`，程序会自动回退到根目录

搜索顺序：

1. `config.ini:last_path`
2. 常见固定路径
3. 非系统盘浅搜索 `WeGameApps`
4. 在已找到的 `WeGameApps` 下直查 `英雄联盟体验服`
5. 有限深度搜索
6. 深度兜底搜索
7. 手动选择目录

### Archive Extraction Rule

只处理归档中前缀为：

```text
league of legends/
```

的条目，大小写不敏感。

若归档中不包含此前缀，注入失败。

### Overwrite Rule

程序支持 3 种覆盖模式：

1. `ASK`
2. `OVERWRITE`
3. `SKIP`

其中 `ASK` 模式下还支持升级状态：

1. 当前文件覆盖
2. 当前文件跳过
3. 后续全部覆盖
4. 后续全部跳过

### Manifest Boundary

后处理只基于本次实际写入成功的文件列表，即 `manifest`。

这意味着：

1. 被跳过的旧文件不会进入 manifest
2. 被跳过的旧 `user.ini` 不会被写入 BOT 卡密
3. 被跳过的旧 `core_cn.dll` 不会被重命名

### Post-Process Rule

后处理规则如下：

1. 若用户输入 BOT 卡密，则将 manifest 中所有命中的 `user.ini` 覆盖写入为该值
2. 将 manifest 中所有命中的 `core_cn.dll` 重命名为 `hid.dll`

后处理不会扫描整个 `Game` 目录。

## Project Structure

```text
src/
├─ app/           # 主流程调度
├─ archive/       # 归档读取与提取
├─ core/          # 类型、常量、错误、上下文
├─ fs/            # 路径与目标目录搜索
├─ io/            # 配置、日志、文本读写
├─ postprocess/   # 后处理逻辑
├─ ui/            # 控制台与对话框交互
└─ win/           # Windows 平台相关封装
```

### app

`app/application.cpp` 负责主流程编排：

1. 初始化控制台
2. 构建上下文
3. 解析归档输入
4. 解析目标目录
5. 获取 BOT 卡密与覆盖模式
6. 执行提取与后处理
7. 保存配置
8. 输出摘要和错误

### archive

`archive_reader.cpp` 使用 `libarchive` 统一读取 `.zip` 和 `.rar`。

`archive_extractor.cpp` 负责：

1. 过滤 `league of legends/`
2. 计算相对路径
3. 安全映射到目标 `Game` 目录
4. 应用覆盖策略
5. 生成 manifest

### fs

`path_utils.cpp` 提供：

1. 路径规范化
2. 目标目录验证
3. 安全目标路径构造
4. 去重保序

`target_resolver.cpp` 提供多阶段目标目录解析。

### io

`logger.cpp` 提供 UTF-8 日志输出。

`config_store.cpp` 负责读取和写入简单 `key=value` 格式的 `config.ini`。

### postprocess

`post_processor.cpp` 只对 manifest 中实际写入的文件执行：

1. `user.ini` 写 BOT 卡密
2. `core_cn.dll -> hid.dll`

### ui

`console_ui.cpp` 实现控制台提示和输入。

`dialogs.cpp` 使用 `IFileDialog` 提供原生文件/目录选择。

### win

`encoding.cpp` 负责 UTF-8 / UTF-16 转换和 UTF-8 文本读写。

`console.cpp` 负责 Windows 控制台初始化。

## Core Data Structures

### OverwriteState

表示覆盖策略状态：

- `mode`: `Ask / Overwrite / Skip`
- `escalation`: `None / OverwriteAll / SkipAll`

### ExtractResult

用于承载提取结果：

1. `manifest`
2. `stats`
3. `gameDir`

### ConfigValues

当前仅包含：

- `lastPath`

## Encoding Strategy

本工程统一采用明确编码策略，不依赖系统默认代码页。

### Internal Representation

Windows API 和路径内部主要使用宽字符路径。

### File Encoding

1. 日志：UTF-8
2. `config.ini`：UTF-8 with BOM
3. `manifest.txt`：UTF-8 with BOM
4. `user.ini`：UTF-8 without BOM

### Console

程序启动时设置控制台输入输出编码为 UTF-8。

## Error Handling Strategy

### Fatal Errors

以下情况会直接终止运行：

1. 归档文件不存在
2. 归档格式不是 `.zip` / `.rar`
3. 找不到合法目标目录
4. 目标 `Game` 目录不存在
5. 归档中不含 `league of legends/`
6. 归档条目发生路径越界

### Non-Fatal Errors

以下情况会尽量吞掉并继续流程：

1. 清理旧工作目录失败
2. 目录枚举中某些无权限或异常目录

## Security Boundaries

`BuildSafeDestinationPath()` 是解压安全边界核心函数。

它负责阻止：

1. `..` 路径穿越
2. 目标路径逃逸到 `Game` 目录之外

任何未来修改都不应绕过该函数直接构造目标写入路径。

## External Dependencies

### Required

1. CMake 3.20+
2. C++20 编译器（推荐 MSVC）
3. `libarchive`

### Current Integration

当前工程通过 `vcpkg` 接入 `libarchive`。

参考 toolchain：

```text
D:\Program Files\vcpkg\scripts\buildsystems\vcpkg.cmake
```

## Build and Output

推荐构建命令：

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="D:\Program Files\vcpkg\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release
```

生成产物：

```text
build/Release/TfBotInjector.exe
```

## Verified Behavior

当前版本已经完成以下实际验证：

1. Release 编译成功
2. 程序可启动
3. ZIP 注入流程可用
4. RAR 注入流程可用
5. BOT 卡密写入可用
6. `core_cn.dll -> hid.dll` 可用
7. 重复注入场景下 manifest 边界符合当前设计语义

## Future Improvements

后续可以继续增强但当前不是必须项：

1. 增加单元测试
2. 增加更细的日志字段
3. 增加发布脚本或打包说明
4. 增加更显式的 RAR 格式兼容提示
