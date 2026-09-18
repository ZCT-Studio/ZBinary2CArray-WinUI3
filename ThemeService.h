// Licensed under the MIT License
//
// ThemeService — 3-state theme preference (System / Light / Dark) for
// the WinUI3 GUI. When the preference is "System", the service subscribes
// to UISettings.ColorValuesChanged so the app reacts to OS light/dark
// toggles in real time. For Light/Dark, the subscription is dropped — the
// user explicitly chose a side, no need to track the OS.
//
// The icon-button cycling logic lives in MainWindow; this service exposes
// the pure theme state machine + the actual XAML root-element theme
// application.

#ifndef ZBTCA_GUI_THEMESERVICE_H
#define ZBTCA_GUI_THEMESERVICE_H

#include "pch.h"

#include <core/settings.hpp>

#include <atomic>
#include <functional>
#include <mutex>
#include <vector>

namespace gui {

    class ThemeService {
    public:
        using ThemeChangedFn =
            std::function<void(::core::ThemePreference, bool /*is_dark_now*/)>;

        ThemeService();
        ~ThemeService();

        ThemeService(ThemeService const&)            = delete;
        ThemeService& operator=(ThemeService const&)  = delete;

        // Apply a new preference. Calling with the same value is a no-op
        // but still re-applies the theme to the supplied root (idempotent).
        void set_preference(::core::ThemePreference pref);

        [[nodiscard]] ::core::ThemePreference preference() const noexcept;
        // True when the *effective* theme (after System resolution) is dark.
        [[nodiscard]] bool is_dark() const noexcept;

        // Apply the effective theme to the supplied XAML root element.
        // Safe to call repeatedly; the GUI typically calls this on
        // every ThemeChanged event and every page navigation.
        void apply_to(::winrt::Microsoft::UI::Xaml::FrameworkElement const& root);

        // Subscribe to effective-theme changes. Listeners are invoked
        // on the DispatcherQueue thread that owns this service.
        void add_changed_listener(ThemeChangedFn fn);

    private:
        // Recompute m_is_dark from preference + UISettings.
        bool compute_is_dark() const;

        // Wire / unwire the OS UISettings listener.
        void start_system_watcher();
        void stop_system_watcher();

        // Marshal a system-side color change back to the dispatcher thread.
        void on_color_values_changed(::winrt::Windows::UI::ViewManagement::UISettings const&,
                                      ::winrt::Windows::Foundation::IInspectable const&);

        ::core::ThemePreference                 m_pref{::core::ThemePreference::System};
        std::atomic<bool>                       m_is_dark{false};
        std::mutex                              m_mutex;
        std::vector<ThemeChangedFn>            m_listeners;

        // The system UI settings + the ColorValuesChanged event token.
        ::winrt::Windows::UI::ViewManagement::UISettings m_ui_settings{nullptr};
        ::winrt::event_token                            m_color_changed_token{};
        bool                                            m_watching{false};

        // Last-applied root — weak by design; ThemeService never keeps
        // a strong reference that would outlive the page.
        ::winrt::weak_ref<::winrt::Microsoft::UI::Xaml::FrameworkElement> m_root{nullptr};
    };

} // namespace gui

#endif // ZBTCA_GUI_THEMESERVICE_H
