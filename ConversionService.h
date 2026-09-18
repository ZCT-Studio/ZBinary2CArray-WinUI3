// Licensed under the MIT License
//
// ConversionService — async wrapper around the ZBinary2CArray library.
//
// The ZBTCA_Bin ctor + ZBTCA_Output::operator() are blocking I/O and
// (in NDEBUG) can return a Response(false) instead of throwing — the
// service isolates both facts behind a single fire-and-forget coroutine
// that returns a structured ConversionResult.
//
// IMPORTANT: this TU #includes "ZBinary2CArray/zbtca.h" which pulls
// <windows.h> under _WIN32. It must NOT be linked into the cross-platform
// `core` library — only into the WinUI3 GUI.

#ifndef ZBTCA_GUI_CONVERSIONSERVICE_H
#define ZBTCA_GUI_CONVERSIONSERVICE_H

#include "pch.h"

#include <ZBinary2CArray/zbtca.h>          // library API
#include <ZBinary2CArray/types.hpp>         // OutputCfg / TypeFlag

#include <core/i18n_manager.hpp>

#include <filesystem>
#include <string>
#include <utility>
#include <memory>

namespace gui {

    struct ConversionRequest {
        std::filesystem::path input_file;     // must exist
        std::filesystem::path output_dir;    // must exist + be writable
        std::string            output_stem;   // filename WITHOUT extension
        ::ZBTCA_Types::OutputCfg cfg;         // snapshot by value
    };

    struct ConversionResult {
        bool                   ok = false;
        std::string            message;          // empty on success, populated on failure
        std::filesystem::path  output_file;     // path actually written
        std::filesystem::path  side_header;     // .h sibling when mode == source

        // Localized status — pre-resolved so the UI thread doesn't need
        // to do anything except bind. Defaults to the raw message.
        std::string            localized_message;
    };

    // Derive the output extension from HeaderOnly, matching the library's
    // own behaviour in output.hpp (HeaderOnly==true  -> .hpp
    //                              HeaderOnly==false -> .cpp + auto .h).
    [[nodiscard]] inline std::string derive_output_extension(
        ::ZBTCA_Types::OutputCfg const& cfg) {
        return cfg.HeaderOnly ? ".hpp" : ".cpp";
    }

    // Build the final ConversionRequest before invoking ConvertAsync.
    // Validates input existence + output directory writability + filename
    // sanity (no <>:"/\\|?*). Returns the first encountered error message
    // (an empty string on success).
    [[nodiscard]] std::string validate_request(ConversionRequest const& req);

    // The async entry point. Captures `req` by value, snapshots `cfg`
    // before the background hop, and runs the entire library pipeline
    // on a worker thread. Resumes on the dispatcher that owns `req`.
    // Returns IAsyncAction (ConversionResult is not a WinRT type, so
    // IAsyncOperation<T> cannot be used). The result is written to
    // *out_result on completion.
    ::winrt::Windows::Foundation::IAsyncAction
    ConvertAsync(ConversionRequest req, std::shared_ptr<ConversionResult> out_result);

} // namespace gui

#endif // ZBTCA_GUI_CONVERSIONSERVICE_H
