//
// Created by wanjiangzhi on 2025/11/21.
//

#ifndef BIN2C_HPP
#define BIN2C_HPP

#include "base.hpp"

namespace Bin2 {
    namespace Type {
        inline TypeFlags u8("unsigned char", Inside::TypeFlags_u8);
        inline TypeFlags u16("unsigned short", Inside::TypeFlags_u16);
        inline TypeFlags u32("unsigned int", Inside::TypeFlags_u32);
        inline TypeFlags u64("unsigned long long", Inside::TypeFlags_u64);
    } // namespace Type

    namespace C {
        inline constexpr auto PROJECT_NAME = "Bin2C";
        inline constexpr auto PROJECT_AUTHOR = "江芷酱紫";

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

                #ifdef CXX_FORMAT
                const std::string timeStr = Tools::format("{:%Y/%m/%d %H:%M}", std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::system_clock::now()});
                #else
                std::time_t now_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                std::tm* tm = std::localtime(&now_time_t);
                const std::string = Tools::format("{}/{}/{} {}:{}", tm->tm_year + 1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min);
                #endif

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
                    "// Bin {} - Generation Tool by {}, author {}.\n\n// Generate by {} at {}.\n\n// In file: \"{}\",\n// Written to: \"{}\".",
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

                std::size_t numBytes = dataStruct->GetSize();
                std::size_t elements = 0;
                auto binData = dataStruct->GetData();
                const std::uint8_t* data = binData.data();

                if (data == nullptr || numBytes == 0) {
                    ofs.close();
                    if (fs::exists(this->m_file)) fs::remove(this->m_file);
                    BIN2FILE_ASSERT(data != nullptr || numBytes != 0, "Invalid or empty data in Bin structure");
                }

                try {
                    if (!cfg.pretty) ofs << TAB;
                    switch (cfg.flag.GetType()) {
                            #define General_Parameters(num) ofs, data, numBytes, cfg.pretty
                            using namespace Inside;
                        case TypeFlags_u8:
                            elements = wrData<std::uint8_t>(General_Parameters(4), 12);
                            break;
                        case TypeFlags_u16:
                            elements = wrData<std::uint16_t>(General_Parameters(4), 8);
                            break;
                        case TypeFlags_u32:
                            elements = wrData<std::uint32_t>(General_Parameters(4), 6);
                            break;
                        case TypeFlags_u64:
                            elements = wrData<std::uint64_t>(General_Parameters(4), 4);
                            break;
                        default:
                            {
                                ofs.close();
                                if (fs::exists(this->m_file)) fs::remove(this->m_file);
                                BIN2FILE_ASSERT(false, "Invalid flag type encountered");
                            }
                            #undef General_Parameters
                    }
                } catch (const std::exception& e) {
                    ofs.close();
                    if (fs::exists(this->m_file)) fs::remove(this->m_file);
                    BIN2FILE_ASSERT(false, std::string("Exception while writing data: ") + e.what());
                }

                ofs << "\n}; // " << fileNameValid << "_end (" << elements
                    << " elements)" << (cfg.source ? "" : protectEnd);

                ofs.close();

                if (cfg.source) {
                    fs::path headerFile = this->m_file.parent_path() / fs::path(this->m_file.has_stem() ? this->m_file.stem().string() + ".h" : "_.h");
                    headString = Tools::format(
                        "// Bin {} - Generation Tool by {}, author {}.\n\n// Generate by {} at {}.\n\n// In file: \"{}\",\n// Written to: \"{}\",",
                        "Header\n// Header \"{}\".",
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

                    h_ofs << "#ifdef __cplusplus" << "\n" << "extern \"C\"{\n";

                    h_ofs << "extern " << constDataKey << "unsigned long long " << fileNameValid
                        << "_len; // " << dataStruct->GetSize() << " bits\n";
                    h_ofs << "extern " << constDataKey << cfg.flag.GetName() << ' ' << fileNameValid
                        << "[]; // " <<  elements << " elements";

                    h_ofs << "\n}";

                    h_ofs << protectEnd;
                }

                return Res();
            }

        protected:
            fs::path m_file;

        private:
            template <typename T>
            std::size_t wrData(
                std::ostream& ofs,
                const std::uint8_t* data,
                const std::size_t byteNum,
                const bool pretty,
                const std::size_t lineNum,
                const char tab = '\t',
                const char space = ' ',
                const char endl = '\n',
                const char comma = ','
            ) const {
                const T* rawData = reinterpret_cast<const T*>(data);
                constexpr bool is_u8 = std::is_same_v<T, std::uint8_t>;
                const std::size_t numElements = byteNum / sizeof(T);
                for (std::size_t i = 0; i < numElements; ++i) {
                    if (pretty) {
                        if (i % lineNum == 0) ofs << tab;
                        ofs << "0x"
                            << std::hex << std::uppercase
                            << std::setw(sizeof(T) * 2)
                            << std::setfill('0')
                            << (is_u8 ? static_cast<std::uint32_t>(rawData[i]) : rawData[i]);

                        if (i != numElements - 1) ofs << comma;
                        if ((i + 1) % lineNum == 0) ofs << endl;
                        else ofs << space;
                    } else {
                        if constexpr (is_u8)
                            ofs << static_cast<std::int16_t>(rawData[i]); // 宽化
                        else ofs << rawData[i];

                        if (i != numElements - 1) ofs << comma;
                    }
                }
                return numElements;
            }
        };
    }
} // namespace Bin2C
namespace Bin2C = Bin2::C;

#endif // BIN2C_HPP
