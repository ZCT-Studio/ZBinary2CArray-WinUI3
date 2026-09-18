// Licensed under the MIT License
//
// UTF-8 <-> winrt::hstring bridge using Win32 MultiByteToWideChar /
// WideCharToMultiByte. The `core` layer stores strings as std::string
// (UTF-8) for portability; the GUI layer needs hstring for XAML data
// binding. All conversions happen here so that no Win32 call leaks
// into the cross-platform layer.

#ifndef ZBTCA_GUI_STRINGCONVERT_H
#define ZBTCA_GUI_STRINGCONVERT_H

#include "pch.h"

#include <string>
#include <string_view>

namespace gui {

    // Convert UTF-8 std::string_view -> winrt::hstring.
    // Returns an empty hstring on empty input. Throws std::runtime_error
    // if the system has no UTF-8 codepage available (extremely unlikely).
    [[nodiscard]] inline winrt::hstring to_hstring(std::string_view utf8) {
        if (utf8.empty()) return winrt::hstring{};
        int wide_len = ::MultiByteToWideChar(
            CP_UTF8, 0,
            reinterpret_cast<LPCSTR>(utf8.data()),
            static_cast<int>(utf8.size()),
            nullptr, 0);
        if (wide_len <= 0) {
            throw std::runtime_error("MultiByteToWideChar(CP_UTF8) failed");
        }
        std::wstring wide(static_cast<size_t>(wide_len), L'\0');
        wide_len = ::MultiByteToWideChar(
            CP_UTF8, 0,
            reinterpret_cast<LPCSTR>(utf8.data()),
            static_cast<int>(utf8.size()),
            wide.data(), wide_len);
        if (wide_len <= 0) {
            throw std::runtime_error("MultiByteToWideChar(CP_UTF8) conversion failed");
        }
        return winrt::hstring(wide);
    }

    // Convert winrt::hstring -> UTF-8 std::string.
    [[nodiscard]] inline std::string to_utf8(winrt::hstring const& hstr) {
        if (hstr.empty()) return std::string{};
        int utf8_len = ::WideCharToMultiByte(
            CP_UTF8, 0,
            hstr.c_str(),
            static_cast<int>(hstr.size()),
            nullptr, 0, nullptr, nullptr);
        if (utf8_len <= 0) {
            throw std::runtime_error("WideCharToMultiByte(CP_UTF8) failed");
        }
        std::string utf8(static_cast<size_t>(utf8_len), '\0');
        utf8_len = ::WideCharToMultiByte(
            CP_UTF8, 0,
            hstr.c_str(),
            static_cast<int>(hstr.size()),
            utf8.data(),
            utf8_len, nullptr, nullptr);
        if (utf8_len <= 0) {
            throw std::runtime_error("WideCharToMultiByte(CP_UTF8) conversion failed");
        }
        return utf8;
    }

    // Convenience: UTF-8 std::wstring view -> UTF-8 std::string.
    // Used when bridging from native Win32 wide-string paths to `core`.
    [[nodiscard]] inline std::string wide_to_utf8(std::wstring_view w) {
        if (w.empty()) return std::string{};
        int utf8_len = ::WideCharToMultiByte(
            CP_UTF8, 0,
            w.data(),
            static_cast<int>(w.size()),
            nullptr, 0, nullptr, nullptr);
        if (utf8_len <= 0) {
            throw std::runtime_error("WideCharToMultiByte(CP_UTF8) failed");
        }
        std::string utf8(static_cast<size_t>(utf8_len), '\0');
        utf8_len = ::WideCharToMultiByte(
            CP_UTF8, 0,
            w.data(),
            static_cast<int>(w.size()),
            utf8.data(),
            utf8_len, nullptr, nullptr);
        if (utf8_len <= 0) {
            throw std::runtime_error("WideCharToMultiByte(CP_UTF8) conversion failed");
        }
        return utf8;
    }

    // Convenience: UTF-8 std::string -> std::wstring (for native Win32 APIs).
    [[nodiscard]] inline std::wstring utf8_to_wide(std::string_view utf8) {
        if (utf8.empty()) return std::wstring{};
        int wide_len = ::MultiByteToWideChar(
            CP_UTF8, 0,
            reinterpret_cast<LPCSTR>(utf8.data()),
            static_cast<int>(utf8.size()),
            nullptr, 0);
        if (wide_len <= 0) {
            throw std::runtime_error("MultiByteToWideChar(CP_UTF8) failed");
        }
        std::wstring wide(static_cast<size_t>(wide_len), L'\0');
        wide_len = ::MultiByteToWideChar(
            CP_UTF8, 0,
            reinterpret_cast<LPCSTR>(utf8.data()),
            static_cast<int>(utf8.size()),
            wide.data(), wide_len);
        if (wide_len <= 0) {
            throw std::runtime_error("MultiByteToWideChar(CP_UTF8) conversion failed");
        }
        return wide;
    }

} // namespace gui

#endif // ZBTCA_GUI_STRINGCONVERT_H
