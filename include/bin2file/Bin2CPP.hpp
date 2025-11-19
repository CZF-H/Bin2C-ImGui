//
// Created by wanjiangzhi on 2025/10/21.
//

#ifndef BIN2C_HPP
#define BIN2C_HPP

#include "base.hpp"

namespace Bin2::C {
    inline constexpr auto PROJECT_NAME = "Bin2C";
    inline constexpr auto PROJECT_AUTHOR = "江芷酱紫";

    namespace Inside {
        enum TypeFlags : uint8_t {
            TypeFlags_u8,
            TypeFlags_u16,
            TypeFlags_u32,
            TypeFlags_u64,
        };
    }

    struct TypeFlags {
        TypeFlags(const char* tName, const Inside::TypeFlags t)
            : m_name(tName),
              m_flag(t) {}

        [[nodiscard]] const char* GetName() const { return m_name; }
        [[nodiscard]] Inside::TypeFlags GetType() const { return m_flag; }
        operator const char*() const { return m_name; }     // NOLINT(*-explicit-constructor)
        operator Inside::TypeFlags() const { return m_flag; } // NOLINT(*-explicit-constructor)

    private:
        const char* m_name;
        Inside::TypeFlags m_flag;
    };

    namespace Type {
        inline TypeFlags u8("unsigned char", Inside::TypeFlags_u8);
        inline TypeFlags u16("unsigned short", Inside::TypeFlags_u16);
        inline TypeFlags u32("unsigned int", Inside::TypeFlags_u32);
        inline TypeFlags u64("unsigned long long", Inside::TypeFlags_u64);
    } // namespace Type

    enum ExportType : uint8_t {
        ExportType_None,
        ExportType_static,
        ExportType_inline
    };
    
    struct Output {
        struct Config {
            ~Config() = default;

            TypeFlags flag = Type::u8;
            ExportType exportType = ExportType_None;
            bool pretty = false;
            bool constant = false;
            bool source = false;

            static Config Default() { return {}; }
        };

        explicit Output(fs::path file) : m_file(std::move(file)) {}

        explicit Output() = default;

        explicit Output(const Bin* dataStruct, const bool isSRC = false) {
            if (!dataStruct) throw std::invalid_argument("dataStruct is nullptr");
            if (dataStruct->DeclareOnly()) throw std::invalid_argument("dataStruct is null");
            const auto p = dataStruct->GetFile().string() + (isSRC ? ".c" : ".h");
            m_file = std::move(fs::path(p));
        }

        explicit Output(const Bin* dataStruct, const Config& cfg) : Output(dataStruct, cfg.source) {}

        explicit Output(const fs::path& path, const fs::path& name) {
            m_file = std::move(path / name);
        }

        explicit Output(const fs::path& path, const Bin* dataStruct, const bool isSRC = false) {
            if (!dataStruct) throw std::invalid_argument("dataStruct is nullptr");
            if (dataStruct->DeclareOnly()) throw std::invalid_argument("dataStruct is null");
            m_file = std::move(path / fs::path(dataStruct->GetFile().filename().string() + (isSRC ? ".c" : ".h")));
        }

        explicit Output(const fs::path& path, const Bin* dataStruct, const Config& cfg) : Output(
            path,
            dataStruct,
            cfg.source
        ) {}

        ~Output() = default;

        Res operator()(
            Bin* dataStruct,
            const Config& cfg = Config::Default()
        ) const {
            static const std::string TAB(4, ' ');
            static const std::string SPACE(" ");
            static const std::string ENDL("\n");

            BIN2FILE_ASSERT(dataStruct, "dataStruct is nullptr");
            BIN2FILE_ASSERT(!dataStruct->DeclareOnly(), "dataStruct is null");

            const std::string timeStr = Tools::format("{:%Y/%m/%d %H:%M:%S}", std::chrono::system_clock::now());
            const std::string usrName = GetSysUsrName();

            const std::string fileNameValid =
                Tools::makeValidV(dataStruct->GetFile().filename().string());
            const std::string protectStart = Tools::format(
                "\n\n#pragma once\n#ifndef BIN_{}_HEADER\n#define BIN_{}_HEADER",
                fileNameValid,
                fileNameValid
            );
            const std::string protectEnd = Tools::format(
                "\n\n#endif // BIN_{}_HEADER",
                fileNameValid
            );

            std::string headString = Tools::format(
                "// BinC {} - Generation Tool by {}, author {}.\n\n// Generate by {} at {}.\n\n// In file: \"{}\".\n// Written to: \"{}\".",
                cfg.source ? "Source" : "File",
                PROJECT_NAME,
                PROJECT_AUTHOR,
                usrName,
                timeStr,
                fs::absolute(dataStruct->GetFile()).string(),
                fs::absolute(this->m_file).string()
            );

            std::string addBeforeKey;
            if (!cfg.source)
                switch (cfg.exportType) {
                    case ExportType_static: addBeforeKey = "static ";
                        break;
                    case ExportType_inline: addBeforeKey = "inline ";
                        break;
                    default: break;
                }

            std::string constDataKey;
            if (cfg.constant) constDataKey = "const ";

            fs::path targetPath = this->m_file;
            if (fs::path parentDir = targetPath.parent_path(); !parentDir.empty() && !fs::exists(parentDir))
                BIN2FILE_ASSERT(fs::create_directories(parentDir), "Can't create directory: " + parentDir.string());

            std::ofstream ofs(this->m_file);
            BIN2FILE_ASSERT(ofs.is_open(), "Can't open file for writing: " + this->m_file.string());

            ofs << headString << (cfg.source ? "" : protectStart) << "\n\n";

            ofs << addBeforeKey << constDataKey << "unsigned long long " << fileNameValid
                << "_len = " << dataStruct->GetSize() << ";\n";
            ofs << addBeforeKey << constDataKey << cfg.flag.GetName() << SPACE << fileNameValid
                << "[] = {\n";

            size_t numBytes = dataStruct->GetSize();
            auto binData = dataStruct->GetData();
            const uint8_t* rawData = binData.data();

            if (rawData == nullptr || numBytes == 0) {
                ofs.close();
                if (fs::exists(this->m_file)) fs::remove(this->m_file);
                BIN2FILE_ASSERT(rawData != nullptr || numBytes != 0, "Invalid or empty data in Bin structure");
            }

            size_t numElements = 0;
            switch (cfg.flag.GetType()) {
                using namespace Inside;
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
                    BIN2FILE_ASSERT(false, "Invalid flag type encountered");
            }

            if (numElements == 0) {
                ofs.close();
                if (fs::exists(this->m_file)) fs::remove(this->m_file);
                BIN2FILE_ASSERT(false, "No elements to export after byte-to-element conversion");
            }

            try {
                switch (cfg.flag.GetType()) {
                    using namespace Inside;
                    case TypeFlags_u8:
                        {
                            const auto* data = rawData;
                            for (size_t i = 0; i < numElements; ++i) {
                                if (cfg.pretty) {
                                    if (i % 12 == 0) ofs << TAB;
                                    ofs << "0x" << std::hex << std::uppercase << std::setw(2)
                                        << std::setfill('0') << static_cast<unsigned int>(data[i]);
                                    if (i != numElements - 1) ofs << ',';
                                    if ((i + 1) % 12 == 0) ofs << ENDL;
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
                                if (cfg.pretty) {
                                    if (i % 8 == 0) ofs << TAB;
                                    ofs << "0x" << std::hex << std::uppercase << std::setw(4)
                                        << std::setfill('0') << data[i];
                                    if (i != numElements - 1) ofs << ',';
                                    if ((i + 1) % 8 == 0) ofs << ENDL;
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
                                if (cfg.pretty) {
                                    if (i % 6 == 0) ofs << TAB;
                                    ofs << "0x" << std::hex << std::uppercase << std::setw(8)
                                        << std::setfill('0') << data[i];
                                    if (i != numElements - 1) ofs << ", ";
                                    if ((i + 1) % 6 == 0) ofs << ENDL;
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
                                if (cfg.pretty) {
                                    if (i % 4 == 0) ofs << TAB;
                                    ofs << "0x" << std::hex << std::uppercase << std::setw(16)
                                        << std::setfill('0') << data[i];
                                    if (i != numElements - 1) ofs << ",";
                                    if ((i + 1) % 4 == 0) ofs << ENDL;
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
                            BIN2FILE_ASSERT(false, "Invalid flag type encountered");
                        }
                }
            } catch (const std::exception& e) {
                ofs.close();
                if (fs::exists(this->m_file)) fs::remove(this->m_file);
                BIN2FILE_ASSERT(false, std::string("Exception while writing data: ") + e.what());
            }

            ofs << "\n}; // " << fileNameValid << "_end (" << numElements
                << " elements)" << (cfg.source ? "" : protectEnd);

            ofs.close();

            if (cfg.source) {
                fs::path headerFile = fs::absolute(this->m_file.has_stem() ? this->m_file.stem() : "_").string() +
                                      ".h";
                headString = Tools::format(
                "// BinC {} - Generation Tool by {}, author {}.\n\n// Generate by {} at {}.\n\n// In file: \"{}\".\n// Written to: \"{}\".",
                    "Header",
                    PROJECT_NAME,
                    PROJECT_AUTHOR,
                    usrName,
                    timeStr,
                    fs::absolute(dataStruct->GetFile()).string(),
                    fs::absolute(this->m_file).string(),
                    headerFile.string()
                );
                std::ofstream h_ofs(headerFile);
                BIN2FILE_ASSERT(h_ofs.is_open(), "Can't open file for writing: " + headerFile.string());
                h_ofs << headString << protectStart << "\n\n";

                h_ofs << "extern " << constDataKey << "unsigned long long " << fileNameValid
                    << "_len; // " << dataStruct->GetSize() << " bits\n";
                h_ofs << "extern " << constDataKey << cfg.flag.GetName() << ' ' << fileNameValid
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
