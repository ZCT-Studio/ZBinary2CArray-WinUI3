// Licensed under the MIT License
//
// MainWindow.xaml.cpp - code-behind for MainWindow.xaml.

#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "App.xaml.h"

#include "StringConvert.h"

#include <core/i18n_manager.hpp>
#include <core/settings.hpp>

#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.h>

#include <shobjidl.h>
#include <shellapi.h>
#include <microsoft.ui.xaml.window.h>

namespace winrt::ZBinary2CArray_WinUI3::implementation
{
    namespace
    {
        bool checkbox_is_on(winrt::Microsoft::UI::Xaml::Controls::CheckBox const& cb)
        {
            auto opt = cb.IsChecked();
            return opt != nullptr && opt.Value();
        }
    } // anonymous namespace

    MainWindow::MainWindow()
    {
        InitializeComponent();

        // Set m_current_tag from the I18nManager so the language combo
        // shows the correct selection on first paint.
        m_current_tag = core::I18nManager::instance().current_language();

        // Toggle defaults — also set in XAML, but set here to be sure
        // the visual state is correct after InitializeComponent.
        ModeToggle().IsOn(true);
        IncGuardToggle().IsOn(true);
        TidyToggle().IsOn(true);

        // Annotation checkboxes default ON — enables the text boxes.
        AnnotToolCheck().IsChecked(true);
        AnnotRunnerCheck().IsChecked(true);

        // Scroll position save/restore — fixes the scroll-to-top bug
        // that occurs when the window loses and regains focus.
        MainScrollViewer().ViewChanged(
            { this, &MainWindow::on_scroll_view_changed });
        Activated(
            { this, &MainWindow::on_window_activated });

        core::I18nManager::instance().add_language_changed_listener(
            [this](std::string const& tag)
            {
                auto dq = DispatcherQueue();
                if (!dq) return;
                dq.TryEnqueue([this, tag]()
                {
                    m_current_tag = tag;
                    refresh_strings();
                });
            });

        refresh_strings();
    }

    App& MainWindow::app() noexcept
    {
        return *App::instance();
    }

    HWND MainWindow::hwnd() const
    {
        HWND hwnd = nullptr;
        auto native = const_cast<MainWindow*>(this)->try_as<::IWindowNative>();
        if (native) native->get_WindowHandle(&hwnd);
        return hwnd;
    }

    void MainWindow::ApplyTr()
    {
        refresh_strings();
    }

    void MainWindow::refresh_strings()
    {
        m_applying = true;

        auto& tr = core::I18nManager::instance();

        InputGroupLabel().Text(gui::to_hstring(tr.tr("input.group")));
        BrowseInputButton().Content(winrt::box_value(gui::to_hstring(tr.tr("input.browse"))));
        InputPathBox().PlaceholderText(gui::to_hstring(tr.tr("input.placeholder")));

        OutputGroupLabel().Text(gui::to_hstring(tr.tr("output.group")));
        OutputDirLabel().Text(gui::to_hstring(tr.tr("output.dir_label")));
        BrowseDirButton().Content(winrt::box_value(gui::to_hstring(tr.tr("output.dir_browse"))));
        OutputDirBox().PlaceholderText(gui::to_hstring(tr.tr("output.dir_placeholder")));
        FilenameLabel().Text(gui::to_hstring(tr.tr("output.filename_label")));
        OutputStemBox().PlaceholderText(gui::to_hstring(tr.tr("output.filename_placeholder")));
        FilenameInvalidHint().Text(gui::to_hstring(tr.tr("output.filename_invalid")));

        OptionsGroupLabel().Text(gui::to_hstring(tr.tr("options.group")));
        TypeLabel().Text(gui::to_hstring(tr.tr("options.type_label")));
        TypeU8().Content(winrt::box_value(gui::to_hstring(tr.tr("options.type_u8"))));
        TypeU16().Content(winrt::box_value(gui::to_hstring(tr.tr("options.type_u16"))));
        TypeU32().Content(winrt::box_value(gui::to_hstring(tr.tr("options.type_u32"))));
        TypeU64().Content(winrt::box_value(gui::to_hstring(tr.tr("options.type_u64"))));
        ModeLabel().Text(gui::to_hstring(tr.tr("options.mode_label")));
        ModeToggle().OnContent(winrt::box_value(gui::to_hstring(tr.tr("options.mode_header_only"))));
        ModeToggle().OffContent(winrt::box_value(gui::to_hstring(tr.tr("options.mode_source"))));
        IncGuardToggle().Header(winrt::box_value(gui::to_hstring(tr.tr("options.inc_guard"))));
        TidyToggle().Header(winrt::box_value(gui::to_hstring(tr.tr("options.tidy"))));
        StorageLabel().Text(gui::to_hstring(tr.tr("options.storage_label")));
        ConstLabel().Text(gui::to_hstring(tr.tr("options.const_label")));
        NumsPerLineLabel().Text(gui::to_hstring(tr.tr("options.nums_per_line_label")));
        AnnotGroupLabel().Text(gui::to_hstring(tr.tr("options.annotation_group")));
        AnnotToolCheck().Content(winrt::box_value(gui::to_hstring(tr.tr("options.annot_tool"))));
        AnnotRunnerCheck().Content(winrt::box_value(gui::to_hstring(tr.tr("options.annot_runner"))));
        AnnotToolNameBox().PlaceholderText(gui::to_hstring(tr.tr("options.annot_toolname_placeholder")));
        AnnotRunnerNameBox().PlaceholderText(gui::to_hstring(tr.tr("options.annot_runnername_placeholder")));

        ConvertButton().Content(winrt::box_value(gui::to_hstring(tr.tr("convert.button"))));
        OpenExplorerButton().Content(winrt::box_value(gui::to_hstring(tr.tr("result.open_in_explorer"))));

        auto lang_combo = LanguageCombo();
        auto& current = m_current_tag;
        for (uint32_t i = 0; i < lang_combo.Items().Size(); ++i)
        {
            auto item = lang_combo.Items().GetAt(i);
            auto str = winrt::unbox_value<winrt::hstring>(item);
            if (gui::to_utf8(str) == current)
            {
                if (lang_combo.SelectedIndex() != static_cast<int32_t>(i))
                {
                    lang_combo.SelectedIndex(static_cast<int32_t>(i));
                }
                break;
            }
        }

        m_applying = false;
    }

    winrt::fire_and_forget MainWindow::BrowseInput_Click(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        using namespace winrt::Windows::Storage::Pickers;

        auto weak = get_weak();

        FileOpenPicker picker;
        picker.as<::IInitializeWithWindow>()->Initialize(hwnd());
        picker.ViewMode(PickerViewMode::List);
        picker.SuggestedStartLocation(PickerLocationId::ComputerFolder);
        picker.FileTypeFilter().ReplaceAll({ L"*" });

        auto file = co_await picker.PickSingleFileAsync();
        if (!file) co_return;

        if (auto strong = weak.get())
        {
            strong->InputPathBox().Text(file.Path());
            std::wstring stem(file.Name());
            if (auto pos = stem.find_last_of(L'.'); pos != std::wstring::npos)
            {
                stem = stem.substr(0, pos);
            }
            if (strong->OutputStemBox().Text().empty())
            {
                strong->OutputStemBox().Text(stem);
            }
        }
    }

    winrt::fire_and_forget MainWindow::BrowseDir_Click(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        using namespace winrt::Windows::Storage::Pickers;

        auto weak = get_weak();

        FolderPicker picker;
        picker.as<::IInitializeWithWindow>()->Initialize(hwnd());
        picker.SuggestedStartLocation(PickerLocationId::ComputerFolder);
        picker.FileTypeFilter().ReplaceAll({ L"*" });

        auto folder = co_await picker.PickSingleFolderAsync();
        if (!folder) co_return;

        if (auto strong = weak.get())
        {
            strong->OutputDirBox().Text(folder.Path());
        }
    }

    void MainWindow::LanguageCombo_SelectionChanged(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&)
    {
        if (m_applying) return;
        auto idx = LanguageCombo().SelectedIndex();
        if (idx < 0) return;
        auto item = LanguageCombo().Items().GetAt(static_cast<uint32_t>(idx));
        auto str = winrt::unbox_value<winrt::hstring>(item);
        std::string tag = gui::to_utf8(str);
        m_current_tag = tag;
        core::I18nManager::instance().set_language(tag);
    }

    void MainWindow::OutputStem_TextChanged(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::Controls::TextChangedEventArgs const&)
    {
        auto stem = gui::to_utf8(OutputStemBox().Text());
        bool valid = true;
        if (stem.empty()) valid = false;
        // Use unsigned char — UTF-8 bytes >= 0x80 are negative as signed
        // char, which would falsely trigger the c < 32 check.
        for (unsigned char c : stem)
        {
            if (c < 32) { valid = false; break; }
            for (char b : "<>:\"/\\|?*")
            {
                if (c == static_cast<unsigned char>(b)) { valid = false; break; }
            }
            if (!valid) break;
        }
        if (!stem.empty() && (stem.back() == '.' || stem.back() == ' ')) valid = false;
        FilenameInvalidHint().Visibility(
            valid ? winrt::Microsoft::UI::Xaml::Visibility::Collapsed
                  : winrt::Microsoft::UI::Xaml::Visibility::Visible);
    }

    void MainWindow::AnnotTool_Changed(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        AnnotToolNameBox().IsEnabled(checkbox_is_on(AnnotToolCheck()));
    }

    void MainWindow::AnnotRunner_Changed(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        AnnotRunnerNameBox().IsEnabled(checkbox_is_on(AnnotRunnerCheck()));
    }

    ZBTCA_Types::OutputCfg MainWindow::build_cfg()
    {
        using ZBTCA_Types::OutputCfg;

        OutputCfg cfg{};

        switch (TypeRadio().SelectedIndex())
        {
            case 1:  cfg.ExportTypeFlags = ZBTCA_Types::TypeFlags::u16; break;
            case 2:  cfg.ExportTypeFlags = ZBTCA_Types::TypeFlags::u32; break;
            case 3:  cfg.ExportTypeFlags = ZBTCA_Types::TypeFlags::u64; break;
            default: cfg.ExportTypeFlags = ZBTCA_Types::TypeFlags::u8;  break;
        }

        cfg.HeaderOnly = ModeToggle().IsOn();
        cfg.IncGuard   = IncGuardToggle().IsOn();
        cfg.MoreTidy   = TidyToggle().IsOn();

        switch (StorageCombo().SelectedIndex())
        {
            case 1:  cfg.StorageSpecifier = OutputCfg::StorageSpecifier_static; break;
            case 2:  cfg.StorageSpecifier = OutputCfg::StorageSpecifier_inline; break;
            default: cfg.StorageSpecifier = OutputCfg::StorageSpecifier_none;   break;
        }
        switch (ConstCombo().SelectedIndex())
        {
            case 1:  cfg.ConstSpecifier = OutputCfg::ConstSpecifier_const;     break;
            case 2:  cfg.ConstSpecifier = OutputCfg::ConstSpecifier_constexpr; break;
            default: cfg.ConstSpecifier = OutputCfg::ConstSpecifier_none;      break;
        }

        cfg.NumsPerLine = static_cast<int>(NumsPerLineBox().Value());

        cfg.Annotation.Tool   = checkbox_is_on(AnnotToolCheck());
        cfg.Annotation.Runner = checkbox_is_on(AnnotRunnerCheck());
        if (AnnotToolNameBox().IsEnabled())
        {
            cfg.Annotation.ToolName = gui::to_utf8(AnnotToolNameBox().Text());
        }
        if (AnnotRunnerNameBox().IsEnabled())
        {
            cfg.Annotation.RunnerName = gui::to_utf8(AnnotRunnerNameBox().Text());
        }
        cfg.Annotation.File = true;
        cfg.Annotation.Size = true;
        cfg.Annotation.Time = true;
        return cfg;
    }

    gui::ConversionRequest MainWindow::build_request()
    {
        gui::ConversionRequest req{};
        // Construct paths from the wide (UTF-16) hstring directly —
        // std::filesystem::path(std::string) on Windows uses the system
        // codepage (e.g. GBK), not UTF-8, which corrupts Chinese paths.
        req.input_file  = std::filesystem::path(InputPathBox().Text().c_str());
        req.output_dir  = std::filesystem::path(OutputDirBox().Text().c_str());
        req.output_stem = gui::to_utf8(OutputStemBox().Text());
        req.cfg = build_cfg();
        return req;
    }

    void MainWindow::set_busy(bool busy)
    {
        ConvertButton().IsEnabled(!busy);
        ConvertProgressRing().IsActive(busy);
        ConvertProgressRing().Visibility(
            busy ? winrt::Microsoft::UI::Xaml::Visibility::Visible
                 : winrt::Microsoft::UI::Xaml::Visibility::Collapsed);
        StatusText().Visibility(
            busy ? winrt::Microsoft::UI::Xaml::Visibility::Visible
                 : winrt::Microsoft::UI::Xaml::Visibility::Collapsed);
        if (busy)
        {
            StatusText().Text(gui::to_hstring(
                core::I18nManager::instance().tr("convert.in_progress")));
        }
    }

    void MainWindow::show_result(bool ok, std::string const& message,
                                  std::filesystem::path const& file)
    {
        using winrt::Microsoft::UI::Xaml::Controls::InfoBarSeverity;
        ResultInfoBar().Severity(
            ok ? InfoBarSeverity::Success : InfoBarSeverity::Error);
        ResultInfoBar().Message(gui::to_hstring(message));
        ResultInfoBar().IsOpen(true);

        OpenExplorerButton().Visibility(
            ok ? winrt::Microsoft::UI::Xaml::Visibility::Visible
               : winrt::Microsoft::UI::Xaml::Visibility::Collapsed);

        ResultCard().Visibility(winrt::Microsoft::UI::Xaml::Visibility::Visible);
        ResultRevealStoryboard().Begin();

        if (ok && !file.empty())
        {
            m_last_output_dir = file.parent_path();
            m_last_output_file = file;
        }
    }

    winrt::fire_and_forget MainWindow::show_result_dialog(
        bool ok, std::string const& message)
    {
        using namespace winrt::Microsoft::UI::Xaml::Controls;

        auto weak = get_weak();

        // This is always called from the UI thread, so XamlRoot is ready.
        auto xaml_root = Content()
            .as<winrt::Microsoft::UI::Xaml::FrameworkElement>()
            .XamlRoot();
        if (!xaml_root) co_return;

        auto& tr = core::I18nManager::instance();
        ContentDialog dialog;
        dialog.Title(winrt::box_value(gui::to_hstring(
            ok ? tr.tr("result.title_success") : tr.tr("result.title_error"))));
        dialog.Content(winrt::box_value(gui::to_hstring(message)));
        dialog.CloseButtonText(gui::to_hstring(tr.tr("common.ok")));
        dialog.XamlRoot(xaml_root);

        co_await dialog.ShowAsync();
    }

    winrt::fire_and_forget MainWindow::Convert_Click(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        auto req = build_request();

        if (std::string err = gui::validate_request(req); !err.empty())
        {
            show_result(false, err, {});
            show_result_dialog(false, err);
            co_return;
        }

        set_busy(true);
        ResultCard().Visibility(winrt::Microsoft::UI::Xaml::Visibility::Collapsed);

        auto weak = get_weak();
        auto ui_context = winrt::apartment_context(); // capture UI apartment
        auto result = std::make_shared<gui::ConversionResult>();

        // Switch to a background thread for the heavy conversion work.
        co_await winrt::resume_background();

        try
        {
            co_await gui::ConvertAsync(req, result);
        }
        catch (winrt::hresult_error const& e)
        {
            result->ok = false;
            result->localized_message = core::I18nManager::instance().tr(
                "errors.unexpected", { gui::to_utf8(e.message()) });
        }
        catch (std::exception const& e)
        {
            result->ok = false;
            result->localized_message = core::I18nManager::instance().tr(
                "errors.unexpected", { e.what() });
        }

        // Switch back to the UI apartment to update controls.
        co_await ui_context;

        if (auto strong = weak.get())
        {
            strong->set_busy(false);
            strong->show_result(result->ok, result->localized_message,
                                result->output_file);
            strong->show_result_dialog(result->ok, result->localized_message);
        }
    }

    void MainWindow::OpenExplorer_Click(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        if (m_last_output_file.empty()) return;
        std::wstring wide = m_last_output_file.wstring();
        PIDLIST_ABSOLUTE pidl = nullptr;
        if (SUCCEEDED(::SHParseDisplayName(wide.c_str(), nullptr, &pidl, 0, nullptr))
            && pidl != nullptr)
        {
            // pidl identifies the file — SHOpenFolderAndSelectItems opens
            // the parent folder and selects the file.
            ::SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
            ::CoTaskMemFree(pidl);
        }
        else if (!m_last_output_dir.empty())
        {
            std::wstring dir = m_last_output_dir.wstring();
            ::ShellExecuteW(nullptr, L"explore", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    }

    void MainWindow::ShowFirstRunLanguageDialog()
    {
        auto weak = get_weak();
        auto root = RootGrid();

        // If the XAML tree is already connected, XamlRoot will be non-null
        // and we can show the dialog immediately (deferred one tick so the
        // current call stack unwinds first).
        if (root.XamlRoot())
        {
            DispatcherQueue().TryEnqueue(
                [weak]()
                {
                    if (auto s = weak.get())
                    {
                        auto xaml_root = s->Content()
                            .as<winrt::Microsoft::UI::Xaml::FrameworkElement>()
                            .XamlRoot();
                        if (xaml_root)
                        {
                            s->show_language_dialog_async(xaml_root);
                        }
                    }
                });
            return;
        }

        // Tree not connected yet — wait for the Loaded event, which fires
        // once the element is in the visual tree and XamlRoot is valid.
        root.Loaded(
            [weak](winrt::Windows::Foundation::IInspectable const&,
                   winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
            {
                if (auto s = weak.get())
                {
                    auto xaml_root = s->Content()
                        .as<winrt::Microsoft::UI::Xaml::FrameworkElement>()
                        .XamlRoot();
                    if (xaml_root)
                    {
                        s->show_language_dialog_async(xaml_root);
                    }
                }
            });
    }

    winrt::fire_and_forget MainWindow::show_language_dialog_async(
        winrt::Microsoft::UI::Xaml::XamlRoot xaml_root)
    {
        using namespace winrt::Microsoft::UI::Xaml::Controls;

        auto weak = get_weak();

        ContentDialog dialog;
        dialog.Title(winrt::box_value(gui::to_hstring(
            core::I18nManager::instance().tr("first_run.language_title"))));

        // Friendly language names (self-identifying, no localization needed)
        ComboBox combo;
        combo.Items().Append(winrt::box_value(L"English"));
        combo.Items().Append(winrt::box_value(L"\u7B80\u4F53\u4E2D\u6587"));   // 简体中文
        combo.Items().Append(winrt::box_value(L"\u7E41\u9AD4\u4E2D\u6587"));   // 繁體中文

        // Map current language tag to combo index
        auto current = core::I18nManager::instance().current_language();
        int default_idx = 0;
        if (current == "zh-CN") default_idx = 1;
        else if (current == "zh-TW") default_idx = 2;
        combo.SelectedIndex(default_idx);

        dialog.Content(combo);
        dialog.PrimaryButtonText(gui::to_hstring(
            core::I18nManager::instance().tr("common.ok")));
        dialog.DefaultButton(ContentDialogButton::Primary);
        dialog.XamlRoot(xaml_root);

        auto result = co_await dialog.ShowAsync();
        if (result == ContentDialogResult::Primary)
        {
            static constexpr const char* tags[] = { "en-US", "zh-CN", "zh-TW" };
            int idx = combo.SelectedIndex();
            if (idx >= 0 && idx < 3)
            {
                core::I18nManager::instance().set_language(tags[idx]);
            }
        }

        // Persist settings regardless (marks first-run as done)
        if (auto s = weak.get())
        {
            s->app().save_settings();
        }
    }

    void MainWindow::on_scroll_view_changed(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs const&)
    {
        m_scroll_offset = MainScrollViewer().VerticalOffset();
    }

    void MainWindow::on_window_activated(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::WindowActivatedEventArgs const& args)
    {
        using winrt::Microsoft::UI::Xaml::WindowActivationState;
        if (args.WindowActivationState() != WindowActivationState::Deactivated)
        {
            auto offset = m_scroll_offset;
            auto weak = get_weak();
            DispatcherQueue().TryEnqueue(
                [weak, offset]()
                {
                    if (auto strong = weak.get())
                    {
                        auto vertical = winrt::box_value(offset)
                            .as<winrt::Windows::Foundation::IReference<double>>();
                        strong->MainScrollViewer().ChangeView(
                            nullptr, vertical, nullptr, true);
                    }
                });
        }
    }
} // namespace winrt::ZBinary2CArray_WinUI3::implementation
