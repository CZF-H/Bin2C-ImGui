//
// Created by wanjiangzhi on 2025/10/21.
//

#ifndef BIN2C_HPP
#define BIN2C_HPP

#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifndef NDEBUG
#define BIN2C_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " << #cond << "\n" \
            << "Message: " << msg << "\n" \
            << "File: " << __FILE__ << ", Line: " << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (0)
#else
#define BIN2C_ASSERT(cond, msg) \
        do { \
            if (!(cond)) { \
               return ::Bin2C::Res(false, msg); \
            } \
        } while (0)
#endif

#define BIN2C_MSG(m, r) r
#define BIN2C_MSG_T(m) 1
#define BIN2C_MSG_F(m) 0

namespace Bin2C {
    namespace fs = std::filesystem;
    using baseBin_t = std::vector<uint8_t>;

    inline constexpr auto PROJECT_NAME = "Bin2C";
    inline constexpr auto PROJECT_AUTHOR = "江芷酱紫";

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
    } // namespace Tools

    enum TypeFlags : uint8_t {
        TypeFlags_u8,
        TypeFlags_u16,
        TypeFlags_u32,
        TypeFlags_u64,
    };

    struct TypeFlag {
        TypeFlag(const char* tName, const TypeFlags t)
            : m_name(tName),
              m_flag(t) {}

        [[nodiscard]] const char* GetName() const { return m_name; }
        [[nodiscard]] TypeFlags GetType() const { return m_flag; }
        operator const char*() const { return m_name; } // NOLINT(*-explicit-constructor)
        operator TypeFlags() const { return m_flag; } // NOLINT(*-explicit-constructor)

    private:
        const char* m_name;
        TypeFlags m_flag;
    };

    namespace Type {
        inline TypeFlag u8("unsigned char", TypeFlags_u8);
        inline TypeFlag u16("unsigned short", TypeFlags_u16);
        inline TypeFlag u32("unsigned int", TypeFlags_u32);
        inline TypeFlag u64("unsigned long long", TypeFlags_u64);
    } // namespace Type

    enum ExportType : uint8_t {
        ExportType_None,
        ExportType_static,
        ExportType_inline
    };

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
        [[nodiscard]] bool DeclareOnly() const { return m_data.size() == 0; }
    };

    struct ToFile {
        explicit ToFile(fs::path file) : m_file(std::move(file)) {}
        explicit ToFile() = default;

        explicit ToFile(const Bin* dataStruct, const bool isSRC = false) {
            if (!dataStruct) throw std::invalid_argument("dataStruct is nullptr");
            if (dataStruct->DeclareOnly()) throw std::invalid_argument("dataStruct is null");
            auto p = dataStruct->GetFile().string() + (isSRC ? ".c" : ".h");
            m_file = std::move(fs::path(p));
        }

        explicit ToFile(const fs::path& path, const fs::path& name) {
            m_file = std::move(path / name);
        }

        explicit ToFile(const fs::path& path, const Bin* dataStruct, const bool isSRC = false) {
            if (!dataStruct) throw std::invalid_argument("dataStruct is nullptr");
            if (dataStruct->DeclareOnly()) throw std::invalid_argument("dataStruct is null");
            m_file = std::move(path / fs::path(dataStruct->GetFile().filename().string() + (isSRC ? ".c" : ".h")));
        }

        Res operator()(
            Bin* dataStruct,
            const TypeFlag& flag = Type::u8,
            const bool prettyOutput = true,
            const bool constData = false,
            const bool exportSourceWithHeader = false,
            const ExportType& exportType = ExportType_static
        ) const {
            static const std::string TAB(4, ' ');
            static const std::string SPACE(" ");
            static const std::string NEW_LINE("\n");

            BIN2C_ASSERT(dataStruct, "dataStruct is nullptr");
            BIN2C_ASSERT(!dataStruct->DeclareOnly(), "dataStruct is null");

            const std::string fileNameValid =
                Tools::makeValidV(dataStruct->GetFile().filename().string());
            const std::string protectStart = std::format(
                "\n\n#pragma once\n#ifndef BIN_{}_HEADER\n#define BIN_{}_HEADER",
                fileNameValid,
                fileNameValid
            );
            const std::string protectEnd = std::format(
                "\n\n#endif // BIN_{}_HEADER",
                fileNameValid
            );

            std::string headString = std::format(
                "// {} - Generated by {}, author {}.\n\n// In file: \"{}\".\n// Written to \"{}\".",
                exportSourceWithHeader ? "Source" : "File",
                PROJECT_NAME,
                PROJECT_AUTHOR,
                fs::absolute(dataStruct->GetFile()).string(),
                fs::absolute(this->m_file).string()
            );

            std::string addBeforeKey;
            if (!exportSourceWithHeader)
                switch (exportType) {
                    case ExportType_static: addBeforeKey = "static ";
                        break;
                    case ExportType_inline: addBeforeKey = "inline ";
                        break;
                    default: break;
                }

            std::string constDataKey;
            if (constData) constDataKey = "const ";

            fs::path targetPath = this->m_file;
            if (fs::path parentDir = targetPath.parent_path(); !parentDir.empty() && !fs::exists(parentDir))
                BIN2C_ASSERT(fs::create_directories(parentDir), "Can't create directory: " + parentDir.string());

            std::ofstream ofs(this->m_file);
            BIN2C_ASSERT(ofs.is_open(), "Can't open file for writing: " + this->m_file.string());

            ofs << headString << (exportSourceWithHeader ? "" : protectStart) << "\n\n";

            ofs << addBeforeKey << constDataKey << "unsigned long long " << fileNameValid
                << "_len = " << dataStruct->GetSize() << ";\n";
            ofs << addBeforeKey << constDataKey << flag.GetName() << SPACE << fileNameValid
                << "[] = {\n";

            size_t numBytes = dataStruct->GetSize();
            auto binData = dataStruct->GetData();
            const uint8_t* rawData = binData.data();

            if (rawData == nullptr || numBytes == 0) {
                ofs.close();
                if (fs::exists(this->m_file)) fs::remove(this->m_file);
                BIN2C_ASSERT(rawData != nullptr || numBytes != 0, "Invalid or empty data in Bin structure");
            }

            size_t numElements = 0;
            switch (flag.GetType()) {
                case TypeFlags_u8: numElements = numBytes / sizeof(uint8_t);
                    break;
                case TypeFlags_u16: numElements = numBytes / sizeof(uint16_t);
                    break;
                case TypeFlags_u32: numElements = numBytes / sizeof(uint32_t);
                    break;
                case TypeFlags_u64: numElements = numBytes / sizeof(uint64_t);
                    break;
                default: ofs.close();
                    if (fs::exists(this->m_file)) fs::remove(this->m_file);
                    BIN2C_ASSERT(false, "Invalid flag type encountered");
            }

            if (numElements == 0) {
                ofs.close();
                if (fs::exists(this->m_file)) fs::remove(this->m_file);
                BIN2C_ASSERT(false, "No elements to export after byte-to-element conversion");
            }

            try {
                switch (flag.GetType()) {
                    case TypeFlags_u8:
                        {
                            const auto* data = rawData;
                            for (size_t i = 0; i < numElements; ++i) {
                                if (prettyOutput) {
                                    if (i % 12 == 0) ofs << TAB;
                                    ofs << "0x" << std::hex << std::uppercase << std::setw(2)
                                        << std::setfill('0') << static_cast<unsigned int>(data[i]);
                                    if (i != numElements - 1) ofs << ',';
                                    if ((i + 1) % 12 == 0) ofs << NEW_LINE;
                                    else ofs << SPACE;
                                } else {
                                    ofs << static_cast<short>(data[i]);
                                    if (i != numElements - 1) ofs << ',';
                                }
                            }
                            break;
                        }
                    case TypeFlags_u16:
                        {
                            auto data = reinterpret_cast<const uint16_t*>(rawData);
                            for (size_t i = 0; i < numElements; ++i) {
                                if (prettyOutput) {
                                    if (i % 8 == 0) ofs << TAB;
                                    ofs << "0x" << std::hex << std::uppercase << std::setw(4)
                                        << std::setfill('0') << data[i];
                                    if (i != numElements - 1) ofs << ',';
                                    if ((i + 1) % 8 == 0) ofs << NEW_LINE;
                                    else ofs << SPACE;
                                } else {
                                    ofs << data[i];
                                    if (i != numElements - 1) ofs << ',';
                                }
                            }
                            break;
                        }
                    case TypeFlags_u32:
                        {
                            auto data = reinterpret_cast<const uint32_t*>(rawData);
                            for (size_t i = 0; i < numElements; ++i) {
                                if (prettyOutput) {
                                    if (i % 6 == 0) ofs << TAB;
                                    ofs << "0x" << std::hex << std::uppercase << std::setw(8)
                                        << std::setfill('0') << data[i];
                                    if (i != numElements - 1) ofs << ", ";
                                    if ((i + 1) % 6 == 0) ofs << NEW_LINE;
                                    else ofs << SPACE;
                                } else {
                                    ofs << data[i];
                                    if (i != numElements - 1) ofs << ',';
                                }
                            }
                            break;
                        }
                    case TypeFlags_u64:
                        {
                            auto data = reinterpret_cast<const uint64_t*>(rawData);
                            for (size_t i = 0; i < numElements; ++i) {
                                if (prettyOutput) {
                                    if (i % 4 == 0) ofs << TAB;
                                    ofs << "0x" << std::hex << std::uppercase << std::setw(16)
                                        << std::setfill('0') << data[i];
                                    if (i != numElements - 1) ofs << ",";
                                    if ((i + 1) % 4 == 0) ofs << NEW_LINE;
                                    else ofs << SPACE;
                                } else {
                                    ofs << data[i];
                                    if (i != numElements - 1) ofs << ',';
                                }
                            }
                            break;
                        }
                    default:
                        {
                            ofs.close();
                            if (fs::exists(this->m_file)) fs::remove(this->m_file);
                            BIN2C_ASSERT(false, "Invalid flag type encountered");
                        }
                }
            } catch (const std::exception& e) {
                ofs.close();
                if (fs::exists(this->m_file)) fs::remove(this->m_file);
                BIN2C_ASSERT(false, std::string("Exception while writing data: ") + e.what());
            }

            ofs << "\n}; // " << fileNameValid << "_end (" << numElements
                << " elements)" << (exportSourceWithHeader ? "" : protectEnd);

            ofs.close();

            if (exportSourceWithHeader) {
                fs::path headerFile = fs::absolute(this->m_file.has_stem() ? this->m_file.stem() : "_").string() + ".h";
                headString = std::format(
                    "// {} - Generated by {}, author {}.\n\n// In file: \"{}\".\n// Link \"{}\".\n// Written to \"{}\".",
                    "Header",
                    PROJECT_NAME,
                    PROJECT_AUTHOR,
                    fs::absolute(dataStruct->GetFile()).string(),
                    fs::absolute(this->m_file).string(),
                    headerFile.string()
                );
                std::ofstream h_ofs(headerFile);
                BIN2C_ASSERT(h_ofs.is_open(), "Can't open file for writing: " + headerFile.string());
                h_ofs << headString << protectStart << "\n\n";

                h_ofs << "extern " << constDataKey << "unsigned long long " << fileNameValid
                    << "_len; // " << dataStruct->GetSize() << " bits\n";
                h_ofs << "extern " << constDataKey << flag.GetName() << ' ' << fileNameValid
                    << "[]; // " << numElements << " elements";

                h_ofs << protectEnd;
            }

            return Res();
        }

    protected:
        fs::path m_file;
    };
} // namespace Bin2C

#endif // BIN2C_HPP
