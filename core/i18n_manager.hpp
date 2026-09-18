// Licensed under the MIT License
//
// i18n_manager.hpp — singleton internationalization manager.
//
// Loads locale JSON files keyed by language tag (e.g. "en-US",
// "zh-CN"). Translation lookups use dot-separated keys like
// "input.group". Format placeholders {0}, {1}, ... are substituted
// from the optional args vector.

#ifndef ZBTCA_CORE_I18N_MANAGER_HPP
#define ZBTCA_CORE_I18N_MANAGER_HPP

#include "json.hpp"

#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace core {

    class I18nManager {
    public:
        static I18nManager& instance();

        // Register a language's translation table. Multiple calls with
        // the same tag replace the previous table.
        void load_language(std::string_view tag, JsonValue const& data);

        // Set the active language. Notifies all registered listeners.
        void set_language(std::string const& tag);

        // Get the active language tag.
        [[nodiscard]] std::string current_language() const noexcept;

        // Translate a dot-separated key. Returns the key itself if not found.
        [[nodiscard]] std::string tr(std::string const& key) const;

        // Translate with positional {0}, {1}, ... format substitution.
        [[nodiscard]] std::string tr(std::string const& key,
                                     std::vector<std::string> const& args) const;

        // Register a listener fired on language change.
        void add_language_changed_listener(
            std::function<void(std::string const&)> fn);

    private:
        I18nManager() = default;
        I18nManager(I18nManager const&)            = delete;
        I18nManager& operator=(I18nManager const&)  = delete;

        // Recursively flatten a nested JSON object into dot-separated keys.
        void flatten(JsonValue const& v,
                     std::string const& prefix,
                     std::unordered_map<std::string, std::string>& out) const;

        mutable std::mutex m_mutex;
        std::string m_current_tag{ "en-US" };
        std::unordered_map<std::string,
                           std::unordered_map<std::string, std::string>> m_tables;
        std::vector<std::function<void(std::string const&)>> m_listeners;
    };

} // namespace core

#endif // ZBTCA_CORE_I18N_MANAGER_HPP
