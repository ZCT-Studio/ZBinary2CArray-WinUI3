#pragma once

#include "MainWindow.g.h"

#include "ThemeService.h"
#include "ConversionService.h"

#include <core/i18n_manager.hpp>
#include <core/settings.hpp>

#include <ZBinary2CArray/types.hpp>

#include <filesystem>
#include <string>

namespace winrt::ZBinary2CArray_WinUI3::implementation
{
    struct App; // forward declaration - defined in App.xaml.h
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        // Public entry so App can refresh strings once after the window
        // is activated. Safe to call repeatedly.
        void ApplyTr();

        // Show the first-run language selection dialog.
        // Waits for the XAML visual tree to be ready (XamlRoot non-null)
        // before showing the ContentDialog, falling back to the Loaded
        // event if the tree isn't connected yet.
        void ShowFirstRunLanguageDialog();

        // XAML event handlers
        winrt::fire_and_forget BrowseInput_Click(
            winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
        winrt::fire_and_forget BrowseDir_Click(
            winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
        winrt::fire_and_forget Convert_Click(winrt::Windows::Foundation::IInspectable const&,
                            winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OpenExplorer_Click(winrt::Windows::Foundation::IInspectable const&,
                                 winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);

        void LanguageCombo_SelectionChanged(winrt::Windows::Foundation::IInspectable const&,
                                             winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OutputStem_TextChanged(winrt::Windows::Foundation::IInspectable const&,
                                      winrt::Microsoft::UI::Xaml::Controls::TextChangedEventArgs const&);
        void AnnotTool_Changed(winrt::Windows::Foundation::IInspectable const&,
                                winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
        void AnnotRunner_Changed(winrt::Windows::Foundation::IInspectable const&,
                                   winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
        void Telegram_Click(winrt::Windows::Foundation::IInspectable const&,
                            winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);

    private:
        [[nodiscard]] App& app() noexcept;
        [[nodiscard]] HWND hwnd() const;
        [[nodiscard]] gui::ConversionRequest build_request();
        void refresh_strings();
        [[nodiscard]] ZBTCA_Types::OutputCfg build_cfg();
        void show_result(bool ok, std::string const& message,
                          std::filesystem::path const& file);
        winrt::fire_and_forget show_result_dialog(
            bool ok, std::string const& message);
        void set_busy(bool busy);

        // Scroll position save/restore — fixes the scroll-to-top bug
        // that occurs when the window loses and regains focus.
        void on_scroll_view_changed(
            winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs const&);
        void on_window_activated(
            winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::WindowActivatedEventArgs const&);

        // Creates and shows the first-run ContentDialog. Called only after
        // XamlRoot is confirmed available.
        winrt::fire_and_forget show_language_dialog_async(
            winrt::Microsoft::UI::Xaml::XamlRoot xaml_root);

        std::string m_current_tag{ "en-US" };
        bool m_applying{ false };
        std::filesystem::path m_last_output_dir;
        std::filesystem::path m_last_output_file;
        double m_scroll_offset{ 0.0 };
    };
}

namespace winrt::ZBinary2CArray_WinUI3::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
