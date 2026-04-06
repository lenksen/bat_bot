
---

# WeGame ZIP Installer - Code Reading README

## 1. 项目定位

这是一个**单文件 BAT 封装器**，内部嵌入了完整的 **PowerShell 核心脚本**。
它的目的不是“通用解压”，而是：

1. 让用户选择一个 `.zip`
2. 从 ZIP 中**只提取** `league of legends/` 这一棵目录
3. 将其内容释放到目标游戏目录的 `Game` 子目录中
4. 进行两类后处理：
   - 把用户输入的 BOT 卡密写入所有本次写入的 `user.ini`
   - 把所有本次写入的 `core_cn.dll` 重命名为 `hid.dll`

脚本是**交互式**的，不是纯静默模式。

---

## 2. 单文件结构概览

这个文件实际上由两部分组成：

### 2.1 BAT 外壳
职责：

- 创建临时工作目录
- 从自身文件中提取 `#__POWERSHELL_BELOW__` 之后的内容
- 将这些内容写成临时 `core.ps1`
- 用 `powershell.exe -File` 执行 `core.ps1`
- 执行结束后清理临时目录

### 2.2 PowerShell 核心
职责：

- 路径处理
- 配置读写
- 日志
- 目标游戏目录自动搜索
- ZIP 解压
- 冲突覆盖策略
- 后处理
- 错误捕获与日志落盘

---

## 3. 关键术语

理解代码时，先区分下面几个路径概念：

### `ScriptRoot`
BAT 脚本所在目录。
用途：

- 作为默认 ZIP 选择窗口的初始目录
- 存放 `config.ini`
- 存放日志文件 `wg_installer_*.log`

### `WorkDir`
临时工作目录，位于：

```text
%TEMP%\WGZipInstaller\run_<随机戳>\
```

用途：

- 存放临时提取出的 `core.ps1`
- 存放 `manifest.txt`

### `TargetRoot`
目标游戏根目录，语义上应为：

```text
...\英雄联盟体验服
```

注意：
函数 `Get-TargetRoot` 支持用户传入：

- `...\英雄联盟体验服`
- 或 `...\英雄联盟体验服\Game`

但它最终返回的始终是**根目录** `...\英雄联盟体验服`。

### `GameDir`
真正的解压目标目录：

```text
$TargetRoot\Game
```

ZIP 内容最终写入这里。

### `Manifest`
本次实际写入成功的文件清单。
用途：

- 后处理只针对 manifest 中的文件
- 避免误处理未写入/被跳过的文件

---

## 4. 整体执行流程

可把主流程理解为下面这个顺序：

```text
BAT 启动
  -> 创建临时目录
  -> 从自身提取 PowerShell payload 为 core.ps1
  -> 启动 PowerShell 核心脚本

PowerShell 启动
  -> 初始化编码、严格模式、日志变量
  -> 解析 ScriptRoot / WorkDir / ZipPath
  -> 清理旧临时目录
  -> 选择 ZIP
  -> 自动/手动确定 TargetRoot
  -> 读取用户输入的 BOT 卡密
  -> 选择覆盖模式
  -> 从 ZIP 中提取 league of legends/ 到 TargetRoot\Game
  -> 根据 manifest 执行后处理
  -> 更新 config.ini 的 last_path
  -> 输出汇总
  -> 退出
```

---

## 5. BAT 层设计说明

### 5.1 为什么用 BAT + PowerShell 双层结构
目标是发布成一个单文件，用户双击即可运行。

BAT 负责启动和包装，真正逻辑在 PowerShell 中实现。
这种结构避免了单独分发 `.ps1` 文件。

### 5.2 PowerShell payload 提取机制
BAT 中定义了一个标记行：

```bat
#__POWERSHELL_BELOW__
```

BAT 会读取自身文件，找到这一行，然后把它后面的全部文本写到：

```text
%TEMP%\WGZipInstaller\run_xxx\core.ps1
```

然后执行：

```bat
powershell -NoProfile -ExecutionPolicy Bypass -STA -File "%CORE_PS1%"
```

### 5.3 为什么通过环境变量传参
BAT 把这些值写入环境变量：

- `__WG_SCRIPT_DIR`
- `__WG_WORK_DIR`
- `__WG_ZIP_PATH`

PowerShell 核心中如果没有显式参数，就从这些环境变量回填。
这是为了避免在 BAT 中直接拼接复杂命令行参数带来的转义问题。

---

## 6. PowerShell 核心逻辑分层

脚本可以分为 6 个功能层：

1. **日志/输出层**
2. **路径与配置层**
3. **目标目录搜索层**
4. **交互层**
5. **解压层**
6. **后处理层**

---

## 7. 日志与输出层

### 7.1 相关函数

- `Write-Log`
- `Write-Step`
- `Write-InfoLine`
- `Write-WarnLine`
- `Show-Banner`

### 7.2 行为
日志文件位置：

```text
<ScriptRoot>\wg_installer_yyyyMMdd_HHmmss.log
```

日志格式：

```text
[时间戳] [级别] 消息
```

### 7.3 设计目的
- 控制台面向用户
- 日志面向排错和代码维护

---

## 8. 路径与配置层

### 8.1 相关函数

- `Resolve-FullPathStrict`
- `Normalize-PathString`
- `Read-AllLinesAuto`
- `Load-Config`
- `Save-ConfigValues`
- `Get-UniqueOrdered`
- `Get-TargetRoot`

### 8.2 `Get-TargetRoot` 是核心路径规范化函数
它的职责是：

- 输入任意候选路径
- 规范化成绝对路径
- 如果输入以 `\Game` 结尾，则回退到父目录
- 检查该目录下是否存在 `Game` 子目录
- 若有效，返回目标根目录；否则返回 `$null`

所以它定义了**什么才算合法目标目录**。

### 8.3 `config.ini`
脚本只依赖简单的 `key=value` 配置，不支持 section。

当前核心键：

```ini
last_path=E:\WeGameApps\英雄联盟体验服
```

用途：

- 下次启动时优先尝试这个目录
- 如果仍然有效，直接命中

### 8.4 编码处理
`Read-AllLinesAuto` 支持识别：

- UTF-8 BOM
- UTF-16 LE
- UTF-16 BE
- 否则退回系统默认编码

这是为了兼容不同来源的 `config.ini` 或批处理宿主文件。

---

## 9. 目标目录搜索层

这是脚本最复杂的部分。

### 9.1 相关函数

- `Get-FixedDriveObjects`
- `Get-CommonCandidateRoots`
- `Get-ChildDirectoriesSafe`
- `Find-WeGameAppsShallow`
- `Find-LolDirectUnderWeGame`
- `Find-LolLimitedDepth`
- `Find-LolDeepFallback`
- `Prompt-CandidateChoice`
- `Try-Select-ManualTarget`
- `Finish-Resolve`
- `Resolve-TargetRoot`

### 9.2 搜索策略总览

`Resolve-TargetRoot` 的搜索顺序如下：

#### 第 1 步：检查 `config.ini` 的 `last_path`
如果有效，直接返回。

#### 第 2 步：枚举固定盘
用 `Get-FixedDriveObjects` 获取所有 fixed drive，并标记系统盘。

返回对象结构为：

```powershell
[pscustomobject]@{
    Root     = 'D:'
    RootPath = 'D:\'
    IsSystem = $false
}
```

#### 第 3 步：检查常见固定路径
`Get-CommonCandidateRoots` 会根据是否系统盘，拼接一组预设路径，例如：

系统盘常见路径：
- `C:\WeGameApps\英雄联盟体验服`
- `C:\Program Files\WeGameApps\英雄联盟体验服`
- `C:\Program Files (x86)\WeGameApps\英雄联盟体验服`

非系统盘常见路径：
- `D:\WeGameApps\英雄联盟体验服`
- `D:\Tencent\WeGameApps\英雄联盟体验服`
- `D:\Games\WeGameApps\英雄联盟体验服`

#### 第 4 步：仅对非系统盘执行浅搜索
`Find-WeGameAppsShallow -MaxDepth 3`

目标：找到名为 `WeGameApps` 的目录。

#### 第 5 步：对找到的 `WeGameApps` 做直查
直接检查：

```text
<WeGameApps>\英雄联盟体验服
```

#### 第 6 步：有限深度搜索
`Find-LolLimitedDepth -MaxDepth 2`

在 `WeGameApps` 下最多 2 层寻找名为 `英雄联盟体验服` 的目录。

#### 第 7 步：深度兜底搜索
`Find-LolDeepFallback`

仅在**已找到的 `WeGameApps`** 下递归深搜，不会全盘无限扫描。

#### 第 8 步：手动选择目录
如果自动策略都失败，则弹文件夹选择框或命令行输入。

---

## 10. 交互层

### 10.1 ZIP 选择
函数：

- `Ensure-WinForms`
- `Select-ZipFileInteractive`
- `Resolve-ZipInput`

优先行为：

- 使用 `System.Windows.Forms.OpenFileDialog` 弹窗选择 ZIP
- 如果 WinForms 不可用，则退回命令行输入路径

### 10.2 目录选择
函数：

- `Select-TargetFolderInteractive`
- `Try-Select-ManualTarget`

用户可以选：

- `英雄联盟体验服`
- 或其 `Game` 目录

最终都由 `Get-TargetRoot` 统一校验。

### 10.3 覆盖模式选择
函数：

- `Prompt-OverwriteMode`
- `Should-WriteFile`

支持三种模式：

- `ASK`：逐个冲突询问
- `OVERWRITE`：全部覆盖
- `SKIP`：全部跳过已存在文件

在 `ASK` 模式下，单个文件冲突时用户还可以切换为：

- `A`：后续全部覆盖
- `K`：后续全部跳过

---

## 11. ZIP 解压层

### 11.1 相关函数

- `Get-SafeDestinationPath`
- `Copy-ZipEntryToFile`
- `Extract-GameZipSubset`

### 11.2 ZIP 根目录约束
脚本只处理 ZIP 中以：

```text
league of legends/
```

开头的条目，大小写不敏感。

例如：

```text
league of legends/Config/user.ini
league of legends/Bin/core_cn.dll
```

会分别释放到：

```text
<TargetRoot>\Game\Config\user.ini
<TargetRoot>\Game\Bin\core_cn.dll
```

### 11.3 为什么是提取到 `GameDir`
脚本的业务语义是：

- ZIP 中的 `league of legends/` 被视作游戏的 `Game` 内容
- 所以内部相对路径是相对于 `TargetRoot\Game` 展开的

### 11.4 路径安全控制
`Get-SafeDestinationPath` 做了几件重要的安全检查：

1. 统一 `/` 和 `\`
2. 去掉前导反斜杠
3. 禁止 `..\` 路径穿越
4. 禁止异常前缀：
   - `处于关闭状态。`
   - `ECHO 处于关闭状态。`
5. 最终用绝对路径判断目标文件是否仍位于 `BaseDir` 之下

这可以防止恶意 ZIP 越界写文件。

### 11.5 文件覆盖行为
对于每个文件条目：

- 若目标文件不存在，直接写入
- 若存在，调用 `Should-WriteFile` 判断是否覆盖
- 覆盖前尝试把现有文件属性设为 `Normal`
- 写入后尽量保留 ZIP 条目的 `LastWriteTime`

### 11.6 `Extract-GameZipSubset` 的返回值
返回对象结构：

```powershell
[pscustomobject]@{
    Manifest = <string[]>
    Stats    = [pscustomobject]@{
        Total
        New
        Overwritten
        Skipped
    }
    GameDir  = <string>
}
```

---

## 12. 后处理层

### 12.1 相关函数

- `Invoke-PostProcess`

### 12.2 BOT 卡密写入逻辑
如果用户输入了 `BotKey`：

- 在 manifest 对应的已存在文件中，查找文件名为 `user.ini`
- 将其内容直接写成 `BotKey`
- 编码为 **UTF-8 无 BOM**

### 12.3 DLL 重命名逻辑
在 manifest 对应文件中查找：

```text
core_cn.dll
```

对每个命中的文件：

1. 计算同目录下目标文件 `hid.dll`
2. 如果 `hid.dll` 已存在，先删除
3. 把 `core_cn.dll` 重命名为 `hid.dll`

### 12.4 一个很重要的行为细节
后处理只针对 **manifest 中本次真正写入的文件**。
也就是说：

- 被跳过的旧文件，不会被写 BOT 卡密
- 被跳过的旧 `core_cn.dll`，也不会被重命名

这是当前实现的明确语义。

---

## 13. 主流程入口

主逻辑在末尾的 `try { ... } catch { ... }` 中。

大致顺序是：

1. 从函数参数或环境变量读取 `ScriptRoot` / `WorkDir` / `ZipPath`
2. 严格解析为绝对路径
3. 创建 `WorkDir`
4. 设置：
   - `ConfigFile`
   - `LogFile`
   - `ManifestFile`
5. 清理旧临时目录
6. 展示 banner
7. 解析 ZIP
8. 解析目标目录
9. 询问 BOT 卡密
10. 询问覆盖模式
11. 解压
12. 后处理
13. 保存 `last_path`
14. 输出结果并退出 `0`

若出现异常：

- 在控制台打印错误
- 写日志
- 打印日志文件路径
- 退出 `1`

---

## 14. 重要的兼容性修复说明

这份脚本已经合并了两个关键的 PowerShell 5.1 兼容修复。
另一个 AI 在阅读代码时，应该把这些模式视为**有意设计**，不要误删。

### 14.1 `.Count` 修复：统一用 `@(...)` 包裹
PowerShell 5.1 中，函数如果只返回一个对象，变量可能不是数组，而是标量对象。
这时直接访问 `.Count` 会报：

```text
The property 'Count' cannot be found on this object.
```

所以代码中大量出现：

```powershell
$items = @(Some-Function)
if ($items.Count -gt 0) { ... }
```

这是为了保证**单元素返回值也能按数组处理**。

### 14.2 `Split-Path` 参数集修复
PowerShell 5.1 中不能这样写：

```powershell
Split-Path -LiteralPath $p -Parent
```

因为 `-LiteralPath` 和 `-Parent` 参数集不兼容。
因此当前脚本使用：

```powershell
Split-Path -LiteralPath $p
```

来取得父目录。

---

## 15. 函数清单与职责速查

下面按功能汇总函数，方便另一个 AI 快速定位。

### 15.1 输出 / 日志
- `Write-Log`：写日志文件
- `Write-Step`：步骤标题
- `Write-InfoLine`：普通信息
- `Write-WarnLine`：警告信息
- `Show-Banner`：启动横幅

### 15.2 基础工具
- `Resolve-FullPathStrict`：严格绝对路径解析
- `Normalize-PathString`：宽松路径规范化
- `Read-AllLinesAuto`：自动编码读文本
- `Get-UniqueOrdered`：去重且保序，兼容嵌套集合

### 15.3 配置
- `Load-Config`：读取 `key=value`
- `Save-ConfigValues`：更新/追加配置项

### 15.4 目标目录判定
- `Get-TargetRoot`：验证一个路径是否是合法目标根目录

### 15.5 UI / 文件选择
- `Ensure-WinForms`
- `Select-ZipFileInteractive`
- `Select-TargetFolderInteractive`
- `Resolve-ZipInput`

### 15.6 磁盘与搜索
- `Get-FixedDriveObjects`
- `Get-CommonCandidateRoots`
- `Get-ChildDirectoriesSafe`
- `Find-WeGameAppsShallow`
- `Find-LolDirectUnderWeGame`
- `Find-LolLimitedDepth`
- `Find-LolDeepFallback`
- `Prompt-CandidateChoice`
- `Try-Select-ManualTarget`
- `Finish-Resolve`
- `Resolve-TargetRoot`

### 15.7 覆盖控制
- `Prompt-OverwriteMode`
- `Should-WriteFile`

### 15.8 解压
- `Get-SafeDestinationPath`
- `Copy-ZipEntryToFile`
- `Extract-GameZipSubset`

### 15.9 后处理
- `Invoke-PostProcess`

### 15.10 维护性辅助
- `Cleanup-OldWorkDirs`：删除 1 天前的旧临时目录

---

## 16. 文件输入输出总结

### 输入
- BAT 自身文件
- 用户选择的 ZIP
- `config.ini`（可选）
- 用户交互输入：
  - ZIP 路径
  - 目标目录
  - BOT 卡密
  - 覆盖策略

### 输出
- 目标 `Game` 目录中的解压文件
- `<ScriptRoot>\config.ini` 中更新的 `last_path`
- `<ScriptRoot>\wg_installer_*.log`
- `<WorkDir>\manifest.txt`（临时，随后随工作目录一起被删）

---

## 17. 脚本假设与限制

### 17.1 ZIP 必须包含正确根目录
如果 ZIP 中不存在：

```text
league of legends/
```

则会直接报错：

```text
ZIP 中未找到 league of legends 根目录内容。
```

### 17.2 目标目录必须满足 `Root + Game` 结构
即：

```text
...\英雄联盟体验服
└─ Game
```

否则不是合法目标。

### 17.3 自动扫描并不会全盘无限深搜
只有在找到 `WeGameApps` 后才做进一步深搜。
这是为了平衡速度与命中率。

### 17.4 依赖 Windows 图形组件
ZIP / 目录选择优先使用 WinForms 对话框。
如果环境不支持，会自动回退命令行输入。

### 17.5 当前不是无交互静默安装器
默认依赖用户交互，不适合直接作为 CI/自动化无人值守脚本使用。

---

## 18. 对维护者/另一个 AI 的修改建议

### 想修改 ZIP 根目录规则
改这里：

- `Extract-GameZipSubset` 中的：

```powershell
$prefix = 'league of legends/'
```

### 想修改目标目录搜索策略
重点看：

- `Get-CommonCandidateRoots`
- `Find-WeGameAppsShallow`
- `Find-LolLimitedDepth`
- `Find-LolDeepFallback`
- `Resolve-TargetRoot`

### 想修改后处理逻辑
重点看：

- `Invoke-PostProcess`

### 想改成静默模式
需要替换或绕过：

- `Select-ZipFileInteractive`
- `Select-TargetFolderInteractive`
- `Prompt-CandidateChoice`
- `Prompt-OverwriteMode`
- `Should-WriteFile`
- `Read-Host` 获取 BOT 卡密的部分

---

## 19. 建议另一个 AI 的阅读顺序

如果要快速理解代码，建议按这个顺序读：

1. **主 `try/catch`**
   - 先理解总流程
2. **`Resolve-TargetRoot`**
   - 这是目录搜索主脑
3. **`Extract-GameZipSubset`**
   - 这是核心业务逻辑
4. **`Invoke-PostProcess`**
   - 这是收尾逻辑
5. **`Get-TargetRoot` / `Get-SafeDestinationPath`**
   - 这是路径安全与语义核心
6. 最后再看 BAT 外壳部分
   - 理解单文件封装机制

---

## 20. 最简心智模型

可以把整个脚本抽象成一句话：

> “这是一个 BAT 封装的 PowerShell 安装器：找到 `英雄联盟体验服` 根目录，把 ZIP 里的 `league of legends/` 解压到它的 `Game` 下，然后对本次写入的 `user.ini` 和 `core_cn.dll` 做后处理，并把成功路径记到 `config.ini` 里。”

---

如果你愿意，我还可以继续帮你输出两份补充文档：

1. **面向人类维护者的 README（偏使用说明）**
2. **面向 AI/代码分析器的设计文档（含伪代码、数据流、状态流、函数依赖图）**