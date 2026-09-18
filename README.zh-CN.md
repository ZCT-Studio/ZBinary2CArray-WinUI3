# ZBinary2CArray-WinUI3

[English](README.md) | [简体中文](README.zh-CN.md) | [繁體中文](README.zh-TW.md)

一个将二进制文件转换为 C/C++ 源代码数组的 Windows 桌面应用程序。使用 WinUI 3 和 C++/WinRT 构建，采用现代 Windows 11 原生界面，支持完整国际化。

## 功能特性

- 将任意二进制文件转换为 C/C++ 头文件（`.hpp`）或源文件+头文件对（`.cpp` + `.h`）
- 支持 `unsigned char`（u8）、`unsigned short`（u16）、`unsigned int`（u32）和 `unsigned long long`（u64）元素类型
- 可配置存储修饰符（`none`、`static`、`inline`）和 const 限定符（`none`、`const`、`constexpr`）
- Include guard 和整洁格式化选项
- 注解支持：工具名称、运行者名称、文件信息、大小和时间戳
- 可配置每行数字数量（0 = 自动）
- 跟随系统主题（浅色/深色），无需手动切换
- 多语言界面：英语、简体中文、繁体中文
- 首次运行语言选择对话框（默认跟随系统语言）
- 响应式布局：宽窗口双栏，窄窗口堆叠
- 非阻塞异步文件选择器和转换（UI 永不卡顿）
- 窗口焦点切换时保持滚动位置
- 资源管理器集成：打开输出文件夹并选中生成的文件

## 环境要求

- Visual Studio 2022（17.x），需安装以下工作负载：
  - **使用 C++ 的桌面开发**
  - **通用 Windows 平台 (UWP) 开发**（可选，用于 MSIX 打包）
- Windows 10 版本 1809+ 或 Windows 11
- Windows App SDK 1.6+（NuGet 包已包含在项目中）
- C++20 编译器（`/std:c++latest`）

## 项目结构

```
ZBinary2CArray-WinUI3/
├── App.xaml / App.xaml.cpp / App.xaml.h   # 应用入口、设置、语言加载
├── MainWindow.xaml / .cpp / .h            # 主界面窗口及全部控件
├── MainWindow.idl                          # MainWindow 的 WinRT IDL 投影
├── ConversionService.h / .cpp             # 库的异步转换封装
├── ThemeService.h / .cpp                  # 系统主题跟随服务
├── StringConvert.h                        # UTF-8 ↔ UTF-16 字符串转换工具
├── pch.h / pch.cpp                        # 预编译头
├── core/                                  # 国际化与设置
│   ├── json.hpp / json.cpp                # 极简 JSON 解析器
│   ├── settings.hpp / settings.cpp        # AppSettings 模型 + JSON 持久化
│   └── i18n_manager.hpp / i18n_manager.cpp # 单例翻译管理器
├── ZBinary2CArray/                         # 核心转换库（内置）
│   ├── zbtca.h                             # 公共 API 头文件
│   ├── types.hpp                           # OutputCfg, TypeFlag, AnnotationCfg
│   ├── bin.hpp                             # 二进制文件读取器
│   ├── output.hpp                          # C/C++ 数组输出写入器
│   ├── response.hpp                        # 转换响应（状态 + 消息）
│   ├── details.hpp                         # 内部实现细节
│   └── LICENSE.TXT                         # 库许可证
├── Locales/                                # 翻译文件（JSON）
│   ├── en-US.json
│   ├── zh-CN.json
│   └── zh-TW.json
├── Assets/                                 # 应用图标和启动画面
├── Build-AppxBundle.ps1                   # 一键构建脚本（x86/x64/ARM64）
├── .github/workflows/build.yml             # CI/CD 流水线
├── packages.config                         # NuGet 包引用
└── ZBinary2CArray-WinUI3.vcxproj          # MSBuild 项目文件
```

## 构建方法

### Visual Studio

1. 在 Visual Studio 2022 中打开 `ZBinary2CArray-WinUI3.slnx` 解决方案文件。
2. 还原 NuGet 包（应自动完成）。
3. 选择 `x64` 平台和 `Debug` 或 `Release` 配置。
4. 构建项目（`Ctrl+Shift+B`）。

### 命令行

```bat
msbuild ZBinary2CArray-WinUI3.vcxproj /p:Configuration=Debug /p:Platform=x64
```

### 一键打包 APPX

构建 Release 版本的 x86、x64、ARM64 APPX 包并收集到 `AppxBundle/` 文件夹：

```powershell
.\Build-AppxBundle.ps1
```

生成的 `.appx` 文件将位于 `AppxBundle/` 目录下。

## 使用方法

1. 启动应用程序。
2. 首次运行时会出现语言选择对话框（默认为系统语言）。选择语言并确认。
3. 点击输入文件旁的**浏览...**按钮，选择二进制文件。
4. 点击输出目录旁的**浏览...**按钮，选择输出文件夹。
5. 输入输出文件名（不含扩展名）。
6. 根据需要配置选项：
   - **元素类型**：u8、u16、u32 或 u64
   - **输出模式**：仅头文件（`.hpp`）或源文件+头文件（`.cpp` + `.h`）
   - **Include guard**：用 `#ifndef`/`#define`/`#endif` 包裹输出
   - **整洁格式化**：对齐和格式化输出数组
   - **存储修饰符**：`none`、`static` 或 `inline`
   - **Const 限定符**：`none`、`const` 或 `constexpr`
   - **每行数字数量**：0 为自动，或指定数量
   - **注解**：切换工具名称、运行者名称、文件信息、大小和时间戳
7. 点击**转换**按钮生成输出文件。
8. 弹出对话框显示结果（成功时显示输出路径，失败时显示错误信息）。
9. 点击**在资源管理器中打开**以在文件资源管理器中显示生成的文件。

使用右上角的语言选择器可随时切换语言。应用自动跟随系统的浅色/深色主题。

## 设置持久化

设置存储在 JSON 文件中：
- **打包模式**：`ApplicationData.Current.LocalFolder\settings.json`
- **非打包模式**：`%APPDATA%\ZBinary2CArray\settings.json`

设置文件存储所选语言和主题偏好（始终为 `System`）。

## 添加新语言

1. 在 `Locales/` 中创建新的 JSON 文件（如 `ja-JP.json`）。
2. 从 `en-US.json` 复制结构并翻译所有值。
3. 在 `App.xaml.cpp` 的 `kLocaleTags` 中添加语言标签。
4. 在 `MainWindow.xaml` 的 ComboBox 中添加语言名称（或首次运行对话框）。

## CI/CD

GitHub Actions 工作流（`.github/workflows/build.yml`）在每次推送和 Pull Request 时自动构建 x86、x64 和 ARM64 版本。构建产物（APPX 包）会作为可下载资源上传。

当推送以 `v` 开头的标签（如 `v1.0.0`）时，三种架构全部构建成功后会自动创建 GitHub Release 并附上 APPX 包。

## 许可证

MIT 许可证。核心库许可证详见 `ZBinary2CArray/LICENSE.TXT`。
