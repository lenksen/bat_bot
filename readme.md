# 体验服BOT注入器

一个使用 C++20 实现的 Windows 原生 BOT 注入工具，用于将注入包内容写入英雄联盟体验服目录。

正式设计文档见：`docs/DESIGN.md`

## 功能

- 支持选择 `.zip` 和 `.rar` 格式的 BOT 注入包
- 自动搜索或手动选择 `英雄联盟体验服` 根目录
- 将注入包中 `league of legends/` 子树内容写入目标 `Game` 目录
- 支持覆盖确认、全部覆盖、全部跳过三种写入策略
- 提供中文引导式命令行交互，统一展示阶段、状态和结果摘要
- 自动执行注入后处理：
  - 将本次写入的 `user.ini` 填入 BOT 卡密
  - 将本次写入的 `core_cn.dll` 重命名为 `hid.dll`
- 将最后一次成功使用的目标目录写入 `config.ini`
- 输出 UTF-8 日志，便于排查问题

## 构建要求

- Windows
- CMake 3.20+
- C++20 编译器（推荐 MSVC）
- LibArchive
- 推荐通过 `vcpkg` 安装 `libarchive`

## 构建

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="D:\Program Files\vcpkg\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release
```

如果 `libarchive` 通过 `vcpkg` 安装，请确保使用对应的 toolchain 文件重新生成构建目录。

## 运行

可直接双击运行：

```text
build/Release/TfBotInjector.exe
```

也可以通过命令行传入归档文件路径：

```powershell
build/Release/TfBotInjector.exe "C:\path\to\package.zip"
build/Release/TfBotInjector.exe "C:\path\to\package.rar"
```

## 交互体验

程序启动后会使用统一的中文命令行界面引导整个注入流程，主要包含以下阶段：

1. 选择注入包
2. 自动或手动确定体验服目录
3. 输入 BOT 卡密，或直接回车跳过
4. 选择文件冲突处理方式
5. 检查压缩包结构
6. 写入文件并执行注入后处理
7. 输出结果摘要与日志路径

当前命令行界面特性：

1. 统一使用中文提示，减少术语负担
2. 多候选目录时使用编号式选择
3. 文件冲突时使用编号菜单，而不是字母快捷键
4. 每个阶段单独显示，不与前一阶段混在同一屏幕
5. 顶部会保留已确认的关键信息，如注入包、目标目录、BOT 卡密和冲突处理方式
6. 成功和失败结束时都会给出清晰摘要，并停留约 3 秒后自动关闭
7. 使用颜色区分阶段、成功、警告和错误，提升可读性

## 日志

程序会输出 UTF-8 日志文件，除 `INFO` 外，还会记录更详细的 `DEBUG` 级别信息，方便排错。

当前日志会覆盖这些关键内容：

1. 注入包来源与最终路径
2. 目标目录搜索过程、候选目录和命中结果
3. 压缩包条目扫描、目标路径映射与跳过原因
4. 文件覆盖、跳过和新增结果
5. 后处理中的 BOT 卡密写入和 DLL 重命名细节

### 交互预览

下面是一段典型的中文 CLI 交互预览：

```text
体验服 BOT 注入器
帮助你将注入包写入体验服 Game 目录

> 注入包
  你可以直接选择 ZIP 或 RAR 注入包，也可以手动输入路径。
  已选择：D:\Downloads\bot_pack.zip

> 目标目录
  正在检查上次使用的目录...
  未找到上次使用记录，继续自动搜索。
  正在检查常见安装路径...

选择体验服目录

  已找到以下可用目录，请选择一个：

  [1] D:\LoLTest
      推荐，可直接用于注入

  [2] E:\Games\英雄联盟体验服

  [0] 手动选择目录

请输入编号：1

  当前使用：D:\LoLTest

> BOT 卡密
输入 BOT 卡密

  将会写入本次成功注入的 user.ini 文件。
  如果暂时不需要，可以直接回车跳过。

请输入 BOT 卡密：

  已跳过

> 文件冲突处理
选择文件冲突处理方式

  当目标目录中已存在同名文件时，请选择处理方式：

  [1] 逐个确认
      每次遇到冲突时再询问你

  [2] 全部覆盖
      遇到同名文件时直接替换

  [3] 全部跳过
      保留原文件，不进行替换

请输入编号：1

  冲突时逐个确认

> 压缩包检查
  正在检查压缩包内容...
OK 已识别有效目录：league of legends/

> 文件写入
  正在写入文件...
OK 文件写入完成
  新增文件：126
  覆盖文件：12
  跳过文件：8

> 注入后处理
  已跳过 BOT 卡密写入
OK 已将 core_cn.dll 重命名为 hid.dll

> 结果摘要
  状态  成功
  目标目录  D:\LoLTest\Game
  新增文件  126
  覆盖文件  12
  跳过文件  8
  卡密写入  0
  DLL 重命名  1
```

### 文件冲突交互

当你选择“逐个确认”时，遇到同名文件会显示统一的中文决策界面：

```text
发现文件冲突

  以下文件已经存在：
  D:\LoLTest\Game\Config\user.ini

请选择处理方式：

  [1] 覆盖当前文件
  [2] 跳过当前文件
  [3] 覆盖后续所有冲突文件
  [4] 跳过后续所有冲突文件

请输入编号：
```

## 发布

发布时至少需要带上以下文件：

1. `TfBotInjector.exe`
2. `archive.dll`
3. `bz2.dll`
4. `libcrypto-3-x64.dll`
5. `liblzma.dll`
6. `lz4.dll`
7. `zlib1.dll`
8. `zstd.dll`

这些依赖当前位于：

```text
build/Release/
```

建议把 `build/Release` 目录中的可执行文件及其动态库一起打包发布。

## 配置与日志

程序会在可执行文件所在目录读写：

1. `config.ini`
2. `tf_bot_injector_*.log`

其中：

1. `config.ini:last_path` 用于保存上次成功目标目录
2. 日志为 UTF-8 文本，便于排障
