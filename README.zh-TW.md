# ZBinary2CArray-WinUI3

[English](README.md) | [简体中文](README.zh-CN.md) | [繁體中文](README.zh-TW.md)

一個將二進位檔案轉換為 C/C++ 原始碼陣列的 Windows 桌面應用程式。使用 WinUI 3 與 C++/WinRT 建置，採用現代 Windows 11 原生介面，支援完整國際化。

## 功能特性

- 將任意二進位檔案轉換為 C/C++ 標頭檔（`.hpp`）或原始碼+標頭檔對（`.cpp` + `.h`）
- 支援 `unsigned char`（u8）、`unsigned short`（u16）、`unsigned int`（u32）和 `unsigned long long`（u64）元素類型
- 可設定儲存修飾符（`none`、`static`、`inline`）和 const 限定符（`none`、`const`、`constexpr`）
- Include guard 和整潔格式化選項
- 註解支援：工具名稱、執行者名稱、檔案資訊、大小和時間戳記
- 可設定每行數字數量（0 = 自動）
- 跟隨系統主題（淺色/深色），無需手動切換
- 多語言介面：英語、簡體中文、繁體中文
- 首次執行語言選擇對話框（預設跟隨系統語言）
- 響應式佈局：寬視窗雙欄，窄視窗堆疊
- 非阻塞非同步檔案選擇器和轉換（UI 永不卡頓）
- 視窗焦點切換時保持捲動位置
- 檔案總管整合：開啟輸出資料夾並選取生成的檔案

## 環境需求

- Visual Studio 2022（17.x），需安裝以下工作負載：
  - **使用 C++ 的桌面開發**
  - **通用 Windows 平台 (UWP) 開發**（可選，用於 MSIX 封裝）
- Windows 10 版本 1809+ 或 Windows 11
- Windows App SDK 1.6+（NuGet 套件已包含在專案中）
- C++20 編譯器（`/std:c++latest`）

## 專案結構

```
ZBinary2CArray-WinUI3/
├── App.xaml / App.xaml.cpp / App.xaml.h   # 應用入口、設定、語言載入
├── MainWindow.xaml / .cpp / .h            # 主介面視窗及全部控制項
├── MainWindow.idl                          # MainWindow 的 WinRT IDL 投影
├── ConversionService.h / .cpp             # 庫的非同步轉換封裝
├── ThemeService.h / .cpp                  # 系統主題跟隨服務
├── StringConvert.h                        # UTF-8 ↔ UTF-16 字串轉換工具
├── pch.h / pch.cpp                        # 預編譯標頭
├── core/                                  # 國際化與設定
│   ├── json.hpp / json.cpp                # 極簡 JSON 解析器
│   ├── settings.hpp / settings.cpp        # AppSettings 模型 + JSON 持久化
│   └── i18n_manager.hpp / i18n_manager.cpp # 單例翻譯管理器
├── ZBinary2CArray/                         # 核心轉換庫（內建）
│   ├── zbtca.h                             # 公共 API 標頭檔
│   ├── types.hpp                           # OutputCfg, TypeFlag, AnnotationCfg
│   ├── bin.hpp                             # 二進位檔案讀取器
│   ├── output.hpp                          # C/C++ 陣列輸出寫入器
│   ├── response.hpp                        # 轉換回應（狀態 + 訊息）
│   ├── details.hpp                         # 內部實作細節
│   └── LICENSE.TXT                         # 庫授權
├── Locales/                                # 翻譯檔案（JSON）
│   ├── en-US.json
│   ├── zh-CN.json
│   └── zh-TW.json
├── Assets/                                 # 應用圖示和啟動畫面
├── Build-AppxBundle.ps1                   # 一鍵建置腳本（x86/x64/ARM64）
├── .github/workflows/build.yml             # CI/CD 流水線
├── packages.config                         # NuGet 套件參考
└── ZBinary2CArray-WinUI3.vcxproj          # MSBuild 專案檔
```

## 建置方法

### Visual Studio

1. 在 Visual Studio 2022 中開啟 `ZBinary2CArray-WinUI3.slnx` 解決方案檔案。
2. 還原 NuGet 套件（應自動完成）。
3. 選擇 `x64` 平台和 `Debug` 或 `Release` 設定。
4. 建置專案（`Ctrl+Shift+B`）。

### 命令列

```bat
msbuild ZBinary2CArray-WinUI3.vcxproj /p:Configuration=Debug /p:Platform=x64
```

### 一鍵封裝 APPX

建置 Release 版本的 x86、x64、ARM64 APPX 套件並收集到 `AppxBundle/` 資料夾：

```powershell
.\Build-AppxBundle.ps1
```

生成的 `.appx` 檔案將位於 `AppxBundle/` 目錄下。

## 使用方法

1. 啟動應用程式。
2. 首次執行時會出現語言選擇對話框（預設為系統語言）。選擇語言並確認。
3. 點擊輸入檔案旁的**瀏覽...**按鈕，選擇二進位檔案。
4. 點擊輸出目錄旁的**瀏覽...**按鈕，選擇輸出資料夾。
5. 輸入輸出檔案名稱（不含副檔名）。
6. 根據需要設定選項：
   - **元素類型**：u8、u16、u32 或 u64
   - **輸出模式**：僅標頭檔（`.hpp`）或原始碼+標頭檔（`.cpp` + `.h`）
   - **Include guard**：用 `#ifndef`/`#define`/`#endif` 包裹輸出
   - **整潔格式化**：對齊和格式化輸出陣列
   - **儲存修飾符**：`none`、`static` 或 `inline`
   - **Const 限定符**：`none`、`const` 或 `constexpr`
   - **每行數字數量**：0 為自動，或指定數量
   - **註解**：切換工具名稱、執行者名稱、檔案資訊、大小和時間戳記
7. 點擊**轉換**按鈕生成輸出檔案。
8. 彈出對話框顯示結果（成功時顯示輸出路徑，失敗時顯示錯誤訊息）。
9. 點擊**在檔案總管中開啟**以在檔案總管中顯示生成的檔案。

使用右上角的語言選擇器可隨時切換語言。應用程式自動跟隨系統的淺色/深色主題。

## 設定持久化

設定存儲在 JSON 檔案中：
- **封裝模式**：`ApplicationData.Current.LocalFolder\settings.json`
- **非封裝模式**：`%APPDATA%\ZBinary2CArray\settings.json`

設定檔案存儲所選語言和主題偏好（始終為 `System`）。

## 新增語言

1. 在 `Locales/` 中建立新的 JSON 檔案（如 `ja-JP.json`）。
2. 從 `en-US.json` 複製結構並翻譯所有值。
3. 在 `App.xaml.cpp` 的 `kLocaleTags` 中新增語言標籤。
4. 在 `MainWindow.xaml` 的 ComboBox 中新增語言名稱（或首次執行對話框）。

## CI/CD

GitHub Actions 工作流（`.github/workflows/build.yml`）在每次推送和 Pull Request 時自動建置 x86、x64 和 ARM64 版本。建置產物（APPX 套件）會作為可下載資源上傳。

當推送以 `v` 開頭的標籤（如 `v1.0.0`）時，三種架構全部建置成功後會自動建立 GitHub Release 並附上 APPX 套件。

## 相關專案與社群

- [ZBinary2CArray](https://github.com/ZCT-Studio/ZBinary2CArray) — 本應用所基於的 C/C++ 二進位轉陣列核心庫。
- [ZCT Studio Telegram 頻道](https://t.me/ZCT_Studio) — 關注獲取專案最新動態與公告。

## 授權條款

MIT 授權條款。核心庫授權詳見 `ZBinary2CArray/LICENSE.TXT`。
