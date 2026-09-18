// Licensed under the MIT License
//
// settings.hpp — application settings model + JSON persistence.
//
// AppSettings carries the user's language tag and theme preference.
// The struct is trivially serializable to a small JSON file stored in
// the app's roaming data folder.

#ifndef ZBTCA_CORE_SETTINGS_HPP
#define ZBTCA_CORE_SETTINGS_HPP

#include "json.hpp"

#include <filesystem>
#include <string>

namespace core {

    enum class ThemePreference : int {
        System = 0,
        Light  = 1,
        Dark   = 2,
    };

    struct AppSettings {
        std::string    language{ "en-US" };
        ThemePreference theme{ ThemePreference::System };
    };

    // Load settings from a JSON file. Throws on parse errors.
    [[nodiscard]] AppSettings load_settings(std::filesystem::path const& path);

    // Save settings to a JSON file. Best-effort; throws on write errors.
    void save_settings(AppSettings const& settings,
                       std::filesystem::path const& path);

} // namespace core

#endif // ZBTCA_CORE_SETTINGS_HPP
