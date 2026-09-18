#pragma once

#include "App.xaml.g.h"

#include "ThemeService.h"

#include <core/settings.hpp>

#include <filesystem>

namespace winrt::ZBinary2CArray_WinUI3::implementation
{
    struct App : AppT<App>
    {
        App();
        ~App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

        // Singleton access — avoids get_self COM cast issues.
        static App* instance() noexcept;

        [[nodiscard]] gui::ThemeService& theme_service() noexcept { return m_theme; }
        [[nodiscard]] core::AppSettings const& settings() const noexcept { return m_settings; }
        [[nodiscard]] core::AppSettings& mutable_settings() noexcept { return m_settings; }
        [[nodiscard]] std::filesystem::path const& settings_path() const noexcept { return m_settings_path; }

        // Persist settings immediately (used after first-run dialog).
        void save_settings() const;

        // True when this is the first launch (no settings file found).
        [[nodiscard]] bool first_run() const noexcept { return m_first_run; }

    private:
        [[nodiscard]] std::filesystem::path resolve_settings_path() const;
        void load_locales();
        void load_persisted_settings();
        void save_persisted_settings() const;

        gui::ThemeService          m_theme;
        core::AppSettings          m_settings{};
        std::filesystem::path      m_settings_path;
        Microsoft::UI::Xaml::Window m_window{ nullptr };
        bool                       m_first_run{ false };
        static App* s_instance;
    };
}
