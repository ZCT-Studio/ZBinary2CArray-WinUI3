// Licensed under the MIT License
//
// settings.cpp — JSON serialization for AppSettings.

#include "settings.hpp"

#include <fstream>
#include <sstream>

namespace core {

    AppSettings load_settings(std::filesystem::path const& path) {
        std::ifstream ifs(path);
        if (!ifs) return AppSettings{};

        std::stringstream ss;
        ss << ifs.rdbuf();

        JsonValue v = JsonValue::parse(ss.str());
        AppSettings s{};

        if (v.is_object()) {
            if (v.contains("language") && v.at("language").is_string()) {
                s.language = v.at("language").as_string();
            }
            if (v.contains("theme") && v.at("theme").is_number()) {
                int t = v.at("theme").as_int();
                if (t >= 0 && t <= 2) {
                    s.theme = static_cast<ThemePreference>(t);
                }
            }
        }
        return s;
    }

    void save_settings(AppSettings const& settings,
                       std::filesystem::path const& path) {
        JsonValue v;
        v["language"] = JsonValue(settings.language);
        v["theme"]   = JsonValue(static_cast<int>(settings.theme));

        std::ofstream ofs(path);
        if (!ofs) return;
        ofs << v.dump(2);
    }

} // namespace core
