#include "pch.h"
// Licensed under the MIT License
//
// Implementation of ThemeService declared in ThemeService.h.

#include "ThemeService.h"

#include <algorithm>
#include <utility>

namespace gui {

    namespace {

        // Inspect UISettings.UIColorType.Background vs Foreground to decide
        // whether the OS is currently in dark mode.
        bool os_is_dark(::winrt::Windows::UI::ViewManagement::UISettings const& ui) {
            using namespace ::winrt::Windows::UI::ViewManagement;
            try {
                auto bg = ui.GetColorValue(UIColorType::Background);
                auto fg = ui.GetColorValue(UIColorType::Foreground);
                // Background is dark if its luminance is less than the foreground's.
                auto lum = [](::winrt::Windows::UI::Color c) {
                    return (static_cast<int>(c.R) * 299 +
                            static_cast<int>(c.G) * 587 +
                            static_cast<int>(c.B) * 114) / 1000;
                };
                return lum(bg) < lum(fg);
            } catch (::winrt::hresult_error const&) {
                return false;
            }
        }

    } // anonymous namespace

    ThemeService::ThemeService() {
        m_ui_settings = ::winrt::Windows::UI::ViewManagement::UISettings{};
        m_is_dark.store(compute_is_dark());
    }

    ThemeService::~ThemeService() {
        stop_system_watcher();
    }

    bool ThemeService::compute_is_dark() const {
        switch (m_pref) {
            case ::core::ThemePreference::Light: return false;
            case ::core::ThemePreference::Dark:  return true;
            case ::core::ThemePreference::System:
            default:
                return os_is_dark(m_ui_settings);
        }
    }

    void ThemeService::set_preference(::core::ThemePreference pref) {
        bool rewatch = (pref != m_pref);
        m_pref = pref;
        if (rewatch) {
            if (pref == ::core::ThemePreference::System) start_system_watcher();
            else stop_system_watcher();
        }

        bool new_dark = compute_is_dark();
        bool old_dark = m_is_dark.exchange(new_dark);
        bool effective_change = (new_dark != old_dark) || rewatch;

        if (effective_change) {
            // Re-apply to the cached root if any.
            auto strong = m_root.get();
            if (strong) apply_to(strong);

            std::vector<ThemeChangedFn> listeners_copy;
            {
                std::lock_guard<std::mutex> lk(m_mutex);
                listeners_copy = m_listeners;
            }
            for (auto const& fn : listeners_copy) {
                if (fn) fn(m_pref, new_dark);
            }
        }
    }

    ::core::ThemePreference ThemeService::preference() const noexcept {
        return m_pref;
    }

    bool ThemeService::is_dark() const noexcept {
        return m_is_dark.load();
    }

    void ThemeService::apply_to(::winrt::Microsoft::UI::Xaml::FrameworkElement const& root) {
        m_root = ::winrt::weak_ref(root);
        if (!root) return;
        using ::winrt::Microsoft::UI::Xaml::ElementTheme;
        if (m_pref == ::core::ThemePreference::System) {
            // Let the app inherit from Application.RequestedTheme, which
            // defaults to the OS theme. This keeps the window in perfect
            // sync with Windows light/dark without any color mismatch.
            root.RequestedTheme(ElementTheme::Default);
        } else {
            root.RequestedTheme(m_is_dark.load() ? ElementTheme::Dark : ElementTheme::Light);
        }
    }

    void ThemeService::add_changed_listener(ThemeChangedFn fn) {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_listeners.push_back(std::move(fn));
    }

    void ThemeService::start_system_watcher() {
        if (m_watching) return;
        if (!m_ui_settings) return;
        using namespace ::winrt::Windows::UI::ViewManagement;
        using namespace ::winrt::Windows::Foundation;
        m_color_changed_token = m_ui_settings.ColorValuesChanged(
            [this](UISettings const& s, IInspectable const& e) {
                on_color_values_changed(s, e);
            });
        m_watching = true;
    }

    void ThemeService::stop_system_watcher() {
        if (!m_watching || !m_ui_settings) {
            m_watching = false;
            return;
        }
        try {
            m_ui_settings.ColorValuesChanged(m_color_changed_token);
        } catch (::winrt::hresult_error const&) {
            // Best-effort cleanup.
        }
        m_watching = false;
    }

    void ThemeService::on_color_values_changed(
        ::winrt::Windows::UI::ViewManagement::UISettings const&,
        ::winrt::Windows::Foundation::IInspectable const&) {

        // This callback fires on a non-UI thread. Recompute and let
        // listeners marshal themselves as needed; the typical listener
        // is MainWindow which uses DispatcherQueue.TryEnqueue.
        bool new_dark = compute_is_dark();
        bool old_dark = m_is_dark.exchange(new_dark);
        if (old_dark == new_dark) return;

        // Re-apply theme — try to use the dispatcher queue captured via root.
        if (auto strong = m_root.get()) {
            auto dq = strong.DispatcherQueue();
            if (dq) {
                dq.TryEnqueue([this]() {
                    if (auto root = m_root.get()) apply_to(root);
                });
            }
        }

        std::vector<ThemeChangedFn> listeners_copy;
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            listeners_copy = m_listeners;
        }
        for (auto const& fn : listeners_copy) {
            if (fn) fn(m_pref, new_dark);
        }
    }

} // namespace gui
