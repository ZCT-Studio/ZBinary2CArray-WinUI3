#include "pch.h"
// Licensed under the MIT License
//
// Implementation of ConversionService declared in ConversionService.h.

#include "ConversionService.h"
#include "StringConvert.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <system_error>

namespace gui {

    namespace {

        // Windows-forbidden filename chars. Whitespace-only / empty stems
        // are also rejected.
        bool filename_is_valid(std::string const& stem) {
            if (stem.empty()) return false;
            static constexpr char bad[] = R"(<>:"/\|?*)";
            // Use unsigned char — UTF-8 bytes >= 0x80 are negative as
            // signed char, which would falsely trigger the c < 32 check.
            for (unsigned char c : stem) {
                if (c < 32) return false;
                for (char b : bad) if (c == static_cast<unsigned char>(b)) return false;
            }
            // Disallow trailing dot / space (Windows strips them anyway).
            if (stem.back() == '.' || stem.back() == ' ') return false;
            // Reserved names. Library typically prefixes '_' on a leading
            // digit, but reserved names must be caught at our level.
            static constexpr const char* reserved[] = {
                "CON","PRN","AUX","NUL",
                "COM1","COM2","COM3","COM4","COM5","COM6","COM7","COM8","COM9",
                "LPT1","LPT2","LPT3","LPT4","LPT5","LPT6","LPT7","LPT8","LPT9",
            };
            std::string upper(stem.size(), ' ');
            std::transform(stem.begin(), stem.end(), upper.begin(),
                           [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
            for (auto* r : reserved) {
                if (upper == r) return false;
            }
            return true;
        }

    } // anonymous namespace

    std::string validate_request(ConversionRequest const& req) {
        namespace fs = std::filesystem;
        std::error_code ec;

        if (req.input_file.empty() || !fs::exists(req.input_file, ec)) {
            return ::core::I18nManager::instance().tr("errors.no_input");
        }
        if (req.output_dir.empty() || !fs::is_directory(req.output_dir, ec)) {
            return ::core::I18nManager::instance().tr("errors.no_output_dir");
        }
        if (req.output_stem.empty()) {
            return ::core::I18nManager::instance().tr("errors.no_filename");
        }
        if (!filename_is_valid(req.output_stem)) {
            return ::core::I18nManager::instance().tr("errors.invalid_filename");
        }
        return {};
    }

    ::winrt::Windows::Foundation::IAsyncAction
    ConvertAsync(ConversionRequest req, std::shared_ptr<ConversionResult> out_result) {
        // Safety net: if NDEBUG is somehow not defined at compile time,
        // details.hpp will std::abort() on the first failed assertion.
        // Bail out now rather than let the user wonder why the app vanished.
#if !defined(NDEBUG)
#error "NDEBUG must be defined in the GUI build — see gui/Directory.Build.props"
#endif

        // ---- 1. Validate (cheap, runs synchronously before the hop) -----
        *out_result = ConversionResult{};
        if (std::string err = validate_request(req); !err.empty()) {
            out_result->ok = false;
            out_result->message = err;
            out_result->localized_message = std::move(err);
            co_return;
        }

        // ---- 2. Derive output path + side header ----------------------------
        namespace fs = std::filesystem;
        std::string ext = derive_output_extension(req.cfg);
        // Use u8path for the UTF-8 stem — path(std::string) uses the system
        // codepage on Windows, corrupting non-ASCII filenames.
        fs::path out_path = req.output_dir / fs::u8path(req.output_stem + ext);
        out_result->output_file = out_path;
        if (!req.cfg.HeaderOnly) {
            out_result->side_header = req.output_dir / fs::u8path(req.output_stem + ".h");
        }

        // ---- 3. Snapshot cfg by value (const-safe — OutputCfg is POD) -----
        ::ZBTCA_Types::OutputCfg cfg_snapshot = req.cfg;

        // ---- 4. Off-thread: construct Bin + Output on worker stack -------
        co_await ::winrt::resume_background();

        std::string worker_error;
        try {
            // Bin ctor calls Reload() which throws on missing / unreadable file.
            // Output holds `const ZBTCA_Bin&`, so bin must outlive output —
            // both are locals on this worker stack, in the correct order.
            ::ZBTCA_Bin bin(req.input_file);
            ::ZBTCA_Output output(bin);
            output.Config() = cfg_snapshot;

            ::ZBTCA_Response resp = output(out_path);
            if (!resp.status()) {
                worker_error = resp.msg().empty()
                    ? ::core::I18nManager::instance().tr("errors.unexpected", { "unknown" })
                    : resp.msg();
            }
        } catch (std::invalid_argument const& e) {
            worker_error = ::core::I18nManager::instance().tr(
                "errors.read_failed", { e.what() });
        } catch (std::runtime_error const& e) {
            worker_error = ::core::I18nManager::instance().tr(
                "errors.write_failed", { e.what() });
        } catch (std::exception const& e) {
            worker_error = ::core::I18nManager::instance().tr(
                "errors.unexpected", { e.what() });
        }

        // ---- 5. Build final result ----------------------------------------
        out_result->ok = worker_error.empty();
        out_result->message = worker_error;
        if (out_result->ok) {
            // path.string() returns the system codepage (e.g. GBK) on Windows,
            // not UTF-8. Convert from wstring (UTF-16) to UTF-8 so the tr()
            // format placeholder and to_hstring() display correctly.
            out_result->localized_message = ::core::I18nManager::instance().tr(
                "result.success", { gui::wide_to_utf8(out_path.wstring()) });
        } else if (!worker_error.empty()) {
            out_result->localized_message = worker_error;
        }
        co_return;
    }

} // namespace gui
