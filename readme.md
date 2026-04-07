# 体验服BOT注入器

一个使用 C++20 实现的 Windows 原生 BOT 注入工具，用于将注入包内容写入英雄联盟体验服目录。

正式设计文档见：`docs/DESIGN.md`

## 功能

- 支持选择 `.zip` 和 `.rar` 格式的 BOT 注入包
- 自动搜索或手动选择 `英雄联盟体验服` 根目录
- 将注入包中 `league of legends/` 子树内容写入目标 `Game` 目录
- 支持覆盖确认、全部覆盖、全部跳过三种写入策略
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

