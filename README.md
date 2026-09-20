# ZBinary2CArray-WinUI3

[English](README.md) | [简体中文](README.zh-CN.md) | [繁體中文](README.zh-TW.md)

A Windows desktop application that converts binary files into C/C++ source arrays. Built with WinUI 3 and C++/WinRT, featuring a modern Windows 11 native UI with full internationalization support.

## Features

- Convert any binary file to a C/C++ header (`.hpp`) or source + header pair (`.cpp` + `.h`)
- Supports `unsigned char` (u8), `unsigned short` (u16), `unsigned int` (u32), and `unsigned long long` (u64) element types
- Configurable storage specifier (`none`, `static`, `inline`) and const qualifier (`none`, `const`, `constexpr`)
- Include guard and tidy formatting options
- Annotation support: tool name, runner name, file info, size, and timestamp
- Configurable numbers per line (0 = auto)
- System theme following (light/dark) with no manual toggle
- Multilingual UI: English, Simplified Chinese, Traditional Chinese
- First-run language selection dialog (defaults to system language)
- Responsive layout: two-column on wide windows, stacked on narrow windows
- Non-blocking async file pickers and conversion (UI never freezes)
- Scroll position preserved across window focus changes
- Explorer integration: open the output folder and select the generated file

## Requirements

- Visual Studio 2022 (17.x) with the following workloads:
  - **Desktop development with C++**
  - **Universal Windows Platform (UWP) development** (optional, for MSIX packaging)
- Windows 10 version 1809+ or Windows 11
- Windows App SDK 1.6+ (NuGet packages are included in the project)
- C++20 compiler (`/std:c++latest`)

## Project Structure

```
ZBinary2CArray-WinUI3/
├── App.xaml / App.xaml.cpp / App.xaml.h   # Application entry point, settings, locale loading
├── MainWindow.xaml / .cpp / .h            # Main UI window with all controls
├── MainWindow.idl                          # WinRT IDL for MainWindow projection
├── ConversionService.h / .cpp             # Async conversion wrapper around the library
├── ThemeService.h / .cpp                  # System theme following service
├── StringConvert.h                        # UTF-8 ↔ UTF-16 string conversion utilities
├── pch.h / pch.cpp                        # Precompiled header
├── core/                                  # Internationalization and settings
│   ├── json.hpp / json.cpp                # Minimal JSON parser
│   ├── settings.hpp / settings.cpp        # AppSettings model + JSON persistence
│   └── i18n_manager.hpp / i18n_manager.cpp # Singleton translation manager
├── ZBinary2CArray/                         # Core conversion library (bundled)
│   ├── zbtca.h                             # Public API header (umbrella)
│   ├── types.hpp                           # OutputCfg, TypeFlag, AnnotationCfg
│   ├── bin.hpp                             # Binary file reader
│   ├── output.hpp                          # C/C++ array output writer
│   ├── response.hpp                        # Conversion response (status + message)
│   ├── details.hpp                         # Internal implementation details
│   └── LICENSE.TXT                         # Library license
├── Locales/                                # Translation files (JSON)
│   ├── en-US.json
│   ├── zh-CN.json
│   └── zh-TW.json
├── Assets/                                 # App icons and splash screen
├── Build-AppxBundle.ps1                   # One-click build script (x86/x64/ARM64)
├── .github/workflows/build.yml             # CI/CD pipeline
├── packages.config                         # NuGet package references
└── ZBinary2CArray-WinUI3.vcxproj          # MSBuild project file
```

## Building

### Visual Studio

1. Open the solution file `ZBinary2CArray-WinUI3.slnx` in Visual Studio 2022.
2. Restore NuGet packages (should happen automatically).
3. Select `x64` as the platform and `Debug` or `Release` as the configuration.
4. Build the project (`Ctrl+Shift+B`).

### Command Line

```bat
msbuild ZBinary2CArray-WinUI3.vcxproj /p:Configuration=Debug /p:Platform=x64
```

### One-Click APPX Bundle

To build Release APPX packages for all three architectures (x86, x64, ARM64) and collect them into an `AppxBundle/` folder:

```powershell
.\Build-AppxBundle.ps1
```

The output `.appx` files will be placed in `AppxBundle/`.

## Usage

1. Launch the application.
2. On first run, a language selection dialog appears (defaults to your system language). Choose a language and confirm.
3. Click **Browse...** next to the input field to select a binary file.
4. Click **Browse...** next to the output directory to choose where the generated file will be saved.
5. Enter an output file name (without extension).
6. Configure options as needed:
   - **Element type**: u8, u16, u32, or u64
   - **Output mode**: Header only (`.hpp`) or Source + header (`.cpp` + `.h`)
   - **Include guard**: Wrap output in `#ifndef`/`#define`/`#endif`
   - **Tidy formatting**: Align and format the output array
   - **Storage specifier**: `none`, `static`, or `inline`
   - **Const specifier**: `none`, `const`, or `constexpr`
   - **Numbers per line**: 0 for auto, or a specific count
   - **Annotations**: Toggle tool name, runner name, file info, size, and timestamp
7. Click **Convert** to generate the output file.
8. A dialog shows the result (success with the output path, or an error message).
9. Click **Open in Explorer** to reveal the generated file in File Explorer.

Use the language selector in the top-right corner to switch languages at any time. The app follows your system's light/dark theme automatically.

## Settings Persistence

Settings are stored in a JSON file at:
- **Packaged**: `ApplicationData.Current.LocalFolder\settings.json`
- **Unpackaged**: `%APPDATA%\ZBinary2CArray\settings.json`

The settings file stores the selected language and theme preference (always `System`).

## Adding a New Language

1. Create a new JSON file in `Locales/` (e.g., `ja-JP.json`).
2. Copy the structure from `en-US.json` and translate all values.
3. Add the locale tag to `kLocaleTags` in `App.xaml.cpp`.
4. Add the language name to the ComboBox in `MainWindow.xaml` (or the first-run dialog).

## CI/CD

The GitHub Actions workflow (`.github/workflows/build.yml`) automatically builds the project for x86, x64, and ARM64 on every push and pull request. Build artifacts (APPX packages) are uploaded as downloadable assets.

When a tag starting with `v` (e.g., `v1.0.0`) is pushed, all three architectures must succeed before a GitHub Release is created with the APPX packages attached.

## Related Projects & Community

- [ZBinary2CArray](https://github.com/ZCT-Studio/ZBinary2CArray) — The core C/C++ binary-to-array conversion library that this application is built upon.
- [ZCT Studio Telegram Channel](https://t.me/ZCT_Studio) — Follow for project updates and announcements.

## License

MIT License. See `ZBinary2CArray/LICENSE.TXT` for the core library license.
