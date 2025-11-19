//
// Created by wanjiangzhi on 2025/11/19.
//

#ifndef BIN2FILE_HPP
#define BIN2FILE_HPP

#ifdef _MSVC_LANG
#define _cpp_ver _MSVC_LANG
#else
#define _cpp_ver __cplusplus
#endif

#if _cpp_ver < 201703L
#error "Must Support C++ 17"
#endif

#include <string>
#include <optional>
#include <vector>
#include <utility>
#if __has_include(<format>)
#include <format>
#define CXX_FORMAT
#else
#include "third_party/fmt/format.h" // for fmt::format
#include "third_party/fmt/chrono.h" // for time format
#endif
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ios>

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef NDEBUG
#define BIN2FILE_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " << #cond << "\n" \
            << "Message: " << msg << "\n" \
            << "File: " << __FILE__ << ", Line: " << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (0)
#else
#define BIN2FILE_ASSERT(cond, msg) \
        do { \
            if (!(cond)) { \
               return ::Bin2File::Res(false, msg); \
            } \
        } while (0)
#endif

#define BIN2FILE_MSG(m, r) r
#define BIN2FILE_MSG_T(m) 1
#define BIN2FILE_MSG_F(m) 0

namespace Bin2 {
    using env_str_t = std::optional<std::string>;

    inline env_str_t GetENV(const std::string& key) {
        using std::nullopt;
        std::string result;
        #ifdef _MSC_VER
        char* value = nullptr;
        if (_dupenv_s(&value, nullptr, key.c_str()) == 0 && value) {
            result = value;
            free(value);
        } else return nullopt;
        #else
        char* value = getenv(key.c_str());
        if (value) result = value;
        else return nullopt;
        #endif
        return result;
    }

    inline std::string GetSysUsrName() {
        static const std::string defaultName = "User";
        #ifdef _WIN32
        wchar_t username[256];
        DWORD size = std::size(username);

        if (!GetUserNameW(username, &size))
            return defaultName;
        const int size_needed = WideCharToMultiByte(
            CP_UTF8, 0, username, static_cast<int>(size - 1),
            nullptr, 0,nullptr, nullptr
        );

        if (size_needed <= 0)
            return defaultName;

        std::string u8usrName(size_needed, 0);
        const int result = WideCharToMultiByte(
            CP_UTF8, 0, username,
            static_cast<int>(size - 1), &u8usrName[0],
            size_needed, nullptr, nullptr
        );

        if (result <= 0) return defaultName;
        return u8usrName;
        #else
        return usrName =
            GetENV("USER")
            .value_or(
                GetENV("USERNAME")
                .value_or(defaultName)
            );
        #endif
    }

    namespace fs = std::filesystem;
    using baseBin_t = std::vector<uint8_t>;

    namespace Tools {
        inline std::string makeValidV(const std::string& input) {
            std::string result;
            result.reserve(input.size());

            for (const char c : input)
                if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') result += c;
                else result += '_';

            if (!result.empty() && std::isdigit(static_cast<unsigned char>(result[0]))) return "_" + result;

            return result;
        }

        inline bool noFileSuffix(const fs::path& p) {
            const auto& ext = p.extension();
            return ext.empty() || ext == ".";
        }

        #ifdef CXX_FORMAT
            using std::format;
        #else
            using fmt::format;
        #endif

    } // namespace Tools

    struct Res {
        bool m_status;
        std::string m_msg;

        explicit Res(const bool status = true, std::string msg = "")
            : m_status(status),
              m_msg(std::move(msg)) {}

        [[nodiscard]] bool status() const { return m_status; }
        operator bool() const { return status(); } // NOLINT(*-explicit-constructor)

        [[nodiscard]] std::string msg() const { return m_msg; }
    };

    class Bin {
        baseBin_t m_data;

    protected:
        fs::path m_file;
        size_t m_size;

    public:
        explicit Bin() {
            m_file = "";
            m_data.clear();
            m_size = 0;
        }

        explicit Bin(fs::path file) : m_file(std::move(file)) {
            if (!fs::exists(m_file))
                throw std::invalid_argument("File not found: " + m_file.string());
            if (!fs::is_regular_file(m_file))
                throw std::invalid_argument("Path is not a regular file: " + m_file.string());
            std::ifstream ifs(m_file, std::ios::binary);
            if (!ifs) throw std::runtime_error("Failed to open file: " + m_file.string());
            m_size = fs::file_size(m_file);
            m_data = baseBin_t(m_size);
            ifs.read(reinterpret_cast<char*>(m_data.data()), static_cast<std::streamsize>(m_size));
            if (!ifs) throw std::runtime_error("Failed to read file or incomplete read: " + m_file.string());
        }

        ~Bin() {
            m_data.clear();
            m_data.clear();
        }

        void operator()(fs::path file) {
            m_file = std::move(file);

            if (!fs::exists(m_file) || !fs::is_regular_file(m_file)) throw std::invalid_argument("File not found");
            std::ifstream ifs(m_file, std::ios::binary);
            ifs.seekg(0, std::ios::end);
            m_size = ifs.tellg();
            ifs.seekg(0, std::ios::beg);
            m_data = baseBin_t(m_size);
            ifs.read(
                reinterpret_cast<char*>(m_data.data()),
                static_cast<std::streamsize>(m_size)
            );
        }

        [[nodiscard]] baseBin_t GetData() const { return m_data; }
        [[nodiscard]] fs::path GetFile() const { return m_file; }
        [[nodiscard]] size_t GetSize() const { return m_size; }
        [[nodiscard]] bool DeclareOnly() const { return m_data.empty(); }
    };
} // namespace Bin2C

#endif //BIN2FILE_HPP