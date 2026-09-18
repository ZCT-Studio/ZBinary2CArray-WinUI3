// Licensed under the MIT License
//
// App.xaml.cpp - code-behind for App.xaml. Wires locale loading, settings
// persistence, theme service, and MainWindow construction.

#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"

#include "StringConvert.h"

#include <core/i18n_manager.hpp>
#include <core/settings.hpp>

#include <winrt/Windows.Storage.h>

#include <shlobj.h>

#include <fstream>
#include <sstream>

namespace winrt::ZBinary2CArray_WinUI3::implementation
{
    using namespace Microsoft::UI::Xaml;

    App* App::s_instance = nullptr;
    namespace fs = std::filesystem;

    namespace
    {
        constexpr std::string_view kLocaleTags[] = { "en-US", "zh-CN", "zh-TW" };

        core::JsonValue read_locale_file(std::string_view tag)
        {
            wchar_t exe_path[MAX_PATH]{};
            GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
            auto exe_root = fs::path(exe_path).parent_path();
            std::string fname = std::string(tag) + ".json";
            for (auto const& sub : { L"Locales", L"locales" })
            {
                auto p = exe_root / sub / fname;
                std::error_code ec;
                if (!fs::exists(p, ec)) continue;
                std::ifstream ifs(p);
                if (!ifs) continue;
                std::stringstream ss;
                ss << ifs.rdbuf();
                try { return core::JsonValue::parse(ss.str()); }
                catch (...) { continue; }
            }
            return core::JsonValue{};
        }

        // Detect the system UI language and map it to one of our supported
        // locale tags. Falls back to en-US.
        std::string detect_system_language()
        {
            wchar_t locale_name[LOCALE_NAME_MAX_LENGTH]{};
            if (GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH) > 0)
            {
                std::string tag;
                for (wchar_t const* p = locale_name; *p; ++p)
                    tag.push_back(static_cast<char>(*p < 128 ? *p : '?'));

                if (tag.find("zh-Hans") != std::string::npos || tag == "zh-CN")
                    return "zh-CN";
                if (tag.find("zh-Hant") != std::string::npos || tag == "zh-TW")
                    return "zh-TW";
            }
            return "en-US";
        }
    } // anonymous namespace

    App::App()
    {
        s_instance = this;
        InitializeComponent();

        m_settings_path = resolve_settings_path();
        load_persisted_settings();

        // Always follow system theme — no user-selectable theme.
        m_settings.theme = core::ThemePreference::System;
        m_theme.set_preference(m_settings.theme);

        load_locales();

        // When the user picks a different language in the UI, persist it.
        core::I18nManager::instance().add_language_changed_listener(
            [this](std::string const& tag)
            {
                m_settings.language = tag;
                save_persisted_settings();
            });
    }

    App::~App()
    {
        try { save_persisted_settings(); } catch (...) {}
    }

    App* App::instance() noexcept { return s_instance; }

    void App::OnLaunched(LaunchActivatedEventArgs const&)
    {
        m_window = make<MainWindow>();
        if (auto root = m_window.Content().try_as<FrameworkElement>())
        {
            m_theme.apply_to(root);
        }
        m_window.Activate();

        // Refresh strings once the window has rendered. Catches the case
        // where the initial locale load finished before the window existed.
        auto main_window = m_window.as<::winrt::ZBinary2CArray_WinUI3::MainWindow>();
        main_window.ApplyTr();

        // First-run: show the language selection dialog.
        if (m_first_run)
        {
            auto impl = winrt::get_self<implementation::MainWindow>(main_window);
            impl->ShowFirstRunLanguageDialog();
        }
    }

    std::filesystem::path App::resolve_settings_path() const
    {
        namespace fs = std::filesystem;
        try
        {
            using namespace winrt::Windows::Storage;
            auto local = ApplicationData::Current().LocalFolder().Path();
            if (!local.empty())
            {
                fs::path dir(local.c_str());
                std::error_code ec;
                fs::create_directories(dir, ec);
                return dir / "settings.json";
            }
        }
        catch (winrt::hresult_error const&)
        {
            // Unpackaged - fall through.
        }

        PWSTR raw = nullptr;
        if (SUCCEEDED(::SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &raw))
            && raw != nullptr)
        {
            fs::path dir = fs::path(raw) / "ZBinary2CArray";
            std::error_code ec;
            fs::create_directories(dir, ec);
            auto p = dir / "settings.json";
            ::CoTaskMemFree(raw);
            return p;
        }
        if (raw) ::CoTaskMemFree(raw);

        return fs::current_path() / "settings.json";
    }

    void App::load_locales()
    {
        for (auto tag : kLocaleTags)
        {
            auto data = read_locale_file(tag);
            if (!data.is_null())
            {
                core::I18nManager::instance().load_language(tag, data);
            }
        }
        core::I18nManager::instance().set_language(m_settings.language);
    }

    void App::load_persisted_settings()
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        if (!fs::exists(m_settings_path, ec))
        {
            // First run — detect system language as default.
            m_first_run = true;
            m_settings.language = detect_system_language();
            return;
        }
        try
        {
            m_settings = core::load_settings(m_settings_path);
        }
        catch (...)
        {
            // Corrupt file - leave defaults in place.
        }
    }

    void App::save_persisted_settings() const
    {
        namespace fs = std::filesystem;
        try
        {
            std::error_code ec;
            fs::create_directories(m_settings_path.parent_path(), ec);
            core::save_settings(m_settings, m_settings_path);
        }
        catch (...)
        {
            // Best-effort persistence - never crash the destructor.
        }
    }

    void App::save_settings() const
    {
        save_persisted_settings();
    }
} // namespace winrt::ZBinary2CArray_WinUI3::implementation
