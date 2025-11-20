#include <iostream>
#include <string>
#include <filesystem>
#include <optional>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"

#include "imutils/fonts.hpp"

#include "include/NotoSansSC-Regular.h"

#include "include/BS_thread_pool.hpp"
#include "imgui_internal.h"
#include "bin2file/Bin2C.hpp"

#include <GLFW/glfw3.h>

using namespace BS;
namespace fs = std::filesystem;

inline constexpr int WINDOW_SIZE_OFFSET = 2;
inline constexpr int WINDOW_POS_OFFSET = -(WINDOW_SIZE_OFFSET / 2);
inline constexpr ImVec2 APP_WINDOW_DEFAULT_SIZE(1000, 750);

bool initImGui = false;

thread_pool g_BackgroundTask(2);
static std::stringstream g_OSS;
class{}* g_App = nullptr;

static void glfw_error_callback(const int error, const char* description) {
    std::cerr << "GLFW error " << error << " " << description << std::endl;
}

static ImVec4 g_BackgroundColor(0.45f, 0.55f, 0.60f, 1.00f);
static float g_Scale;
GLFWmonitor* g_PrimaryMonitor;
GLFWwindow* g_Window;

using env = std::optional<std::string>;
env GetENV(const std::string& key) {
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

enum 屏幕方向 : bool { 横屏, 竖屏 };
inline 屏幕方向 获取屏幕方向(const ImVec2i& 屏幕尺寸) {
    return 屏幕尺寸.x > 屏幕尺寸.y ? 横屏 : 竖屏;
}

inline ImVec2 ToImVec2(const ImVec2i& o) {
    return { static_cast<float>(o.x), static_cast<float>(o.y )};
}
inline ImVec2i ToImVec2i(const ImVec2& o) {
    return { static_cast<int>(o.x), static_cast<int>(o.y) };
}

void GUI() {
    static ImVec2i WindowSize;
    glfwGetWindowSize(g_Window, &WindowSize.x, &WindowSize.y);

    if (!initImGui) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        ImGui::StyleColorsLight();

        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(g_Scale);
        style.FontScaleDpi = g_Scale;

        io.Fonts->AddFontFromMemoryTTF(
            NotoSansSC_Regular_ttf,
            // 自动退化指针
            static_cast<int>(
                NotoSansSC_Regular_ttf_len
            ),
            // 显式转换
            21.0f,
            nullptr,
            ImUtils::Glyph({
                {0x0020, 0x007F}, // Latin kaomoji basic
                {0x00A0, 0x00FF}, // Latin kaomoji exp
                {0x0400, 0x04FF}, // Russian
                {0x2190, 0x21FF}, // Arrows
                {0x25A0, 0x25FF}, // kaomoji graphics and geometry
                {0x2600, 0x26FF}, // kaomoji miscellaneous
                {0x2700, 0x27BF}, // kaomoji dingbats
                {0x3000, 0x303F}, // CJK kaomoji
                {0x3040, 0x309F}, // Japanese kaomoji exp
                {0x30A0, 0x30FF}, // Japanese kaomoji
                {0x3400, 0x4DBF}, // 生僻字
                {0x4E00, 0x9FFF}, // 中日韩统一表意文字
            })
        );

        io.IniFilename = nullptr;

        ImGui_ImplGlfw_InitForOpenGL(g_Window, true);

        ImGui_ImplOpenGL3_Init("#version 300es");

        initImGui = true;
    }


    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(WINDOW_POS_OFFSET, WINDOW_POS_OFFSET));
    ImGui::SetNextWindowSize(
        ImVec2(
            static_cast<float>(WindowSize.x + WINDOW_SIZE_OFFSET),
            static_cast<float>(WindowSize.y + WINDOW_SIZE_OFFSET)
        )
    );

    static constexpr auto MAIN_WINDOW_FLAG =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    static const auto BottomText = "By 江芷酱紫. ";
    static const auto TelegramChannel = "@ZCYMOD";

    static const std::string HOME_DIR =
            #ifdef _WIN32
            GetENV("USERPROFILE").value_or("C:\\\\")
        #elifdef __ANDROID__
        "/sdcard"
        #elifdef __unix__ || __linux__
        std::getenv("HOME")
        #else
        "."
        #endif
        ; // 语句ended

    static std::string inputFile = fs::absolute(HOME_DIR).string();
    static std::string outputFile = (fs::path(inputFile) / fs::path("Bin2C_Output")).string();
    [[maybe_unused]] static bool _init_once_1 = []() {
        fs::create_directory(outputFile);
        if (!fs::exists(outputFile) || !fs::is_directory(outputFile))
            outputFile = inputFile;
        return false;
    }();

    const fs::path inputFS = inputFile;
    const fs::path outputFS = outputFile;

    static Bin2::Bin inputBin;
    static std::future<Bin2::Res> outputFuture;


    auto IntImVec2 = [](const int x, const int y) {
        return ImVec2(
            static_cast<float>(x),
            static_cast<float>(y)
        );
    };

    [[maybe_unused]] auto IntImVec4 = [](const int x, const int y, const int z, const int w) {
        return ImVec4(
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z),
            static_cast<float>(w)
        );
    };

    auto SeparatorTitle = [](const char* label, const float size = 1.0f) {
        ImGui::SetWindowFontScale(size);
        ImGui::SeparatorText(std::forward<const char*>(label));
        ImGui::Spacing();
        ImGui::SetWindowFontScale(1.0f);
    };

    const ImVec2 ChooseWindowSize = IntImVec2(WindowSize.x / 3 * 2, WindowSize.y / 3 * 2);
    auto ChooseFile = [&ChooseWindowSize](std::string& filename, const char* label = "...", ImVec2 size = {0, 0}) {
        if (size.x <= 0 || size.y <= 0)
            size = ChooseWindowSize;
        ImGui::SetWindowSize("选择文件...##内置选择文件", size);
        if (ImGui::Button(label)) {
            static IGFD::FileDialogConfig cfg;
            if (fs::is_regular_file(filename))
                cfg.path = fs::path(filename).parent_path().string();
            else cfg.path = filename;
            cfg.countSelectionMax = 1;
            cfg.flags = ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_NaturalSorting;
            ImGuiFileDialog::Instance()->OpenDialog(
                "内置选择文件",
                "选择文件...",
                ".*",
                cfg
            );
        }
        if (ImGuiFileDialog::Instance()->Display("内置选择文件", ImGuiWindowFlags_NoResize)) {
            if (ImGuiFileDialog::Instance()->IsOk()) filename = ImGuiFileDialog::Instance()->GetFilePathName();

            ImGuiFileDialog::Instance()->Close();
        }
    };

    auto ChoosePath = [&ChooseWindowSize](std::string& path, const char* label = "...", ImVec2 size = {0, 0}) {
        if (size.x <= 0 || size.y <= 0)
            size = ChooseWindowSize;
        ImGui::SetWindowSize("选择目录...##内置选择目录", size);
        if (ImGui::Button(label)) {
            static IGFD::FileDialogConfig cfg;
            cfg.path = path;
            cfg.countSelectionMax = 1;
            cfg.flags = ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_NaturalSorting;
            ImGuiFileDialog::Instance()->OpenDialog(
                "内置选择目录",
                "选择目录...",
                nullptr,
                cfg
            );
        }
        if (ImGuiFileDialog::Instance()->Display("内置选择目录", ImGuiWindowFlags_NoResize)) {
            if (ImGuiFileDialog::Instance()->IsOk()) path = ImGuiFileDialog::Instance()->GetCurrentPath();

            ImGuiFileDialog::Instance()->Close();
        }
    };



    if (ImGui::Begin("##Main", nullptr, MAIN_WINDOW_FLAG)) {
        if (ImGui::BeginChild("###I/O###", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.6f))) {
            if (ImGui::BeginChild("##Input", ImVec2(ImGui::GetContentRegionAvail().x / 2 - 1.0f, 0), true)) {
                ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.0f, 0.5f));
                SeparatorTitle(" Input 输入", 1.75f);
                ImGui::PopStyleVar();
                ImGui::TextUnformatted("输入文件:");
                ImGui::InputText("##输入", &inputFile);
                ImGui::SameLine();
                ChooseFile(inputFile);
                ImGui::Spacing();
                if (ImGui::BeginChild("###File", ImVec2(), true)) {
                    auto FileSize = [](const fs::path& ifs) {
                        std::uintmax_t bitSize = 0ULL;
                        if (!fs::is_regular_file(ifs))
                            return std::string("-");
                        bitSize = std::filesystem::file_size(ifs);
                        const std::vector<std::string> units = {"B", "KB", "MB", "GB", "TB"};
                        int unit_index = 0;
                        auto size = static_cast<double>(bitSize);

                        while (size >= 1024.0 && unit_index < 4) {
                            size /= 1024.0;
                            unit_index++;
                        }

                        return std::format("{:.2f} {:s}", size, units[unit_index]);
                    };

                    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
                    if (fs::is_regular_file(inputFS)) {
                        ImGui::SeparatorText("文件信息:");

                        ImGui::TextWrapped("文件: %s", inputFS.has_stem() ? inputFS.filename().string().c_str() : "(空)");

                        ImGui::TextWrapped(
                            "类型: %s",
                            inputFS.has_extension() ? inputFS.extension().string().c_str() : "未知文件"
                        );

                        ImGui::TextWrapped("路径:%s", "");
                        ImGui::SameLine();
                        if (fs::is_regular_file(inputFS))
                            ImGui::TextLinkOpenURL(
                                inputFile.c_str(),
                                inputFS.has_parent_path()
                                    ? inputFS.parent_path().string().c_str()
                                    : inputFS.string().c_str()
                            );
                        else
                            ImGui::TextLinkOpenURL(
                                inputFile.c_str(),
                                inputFile.c_str()
                            );

                        ImGui::TextWrapped("大小: %s", FileSize(inputFS).c_str());
                    } else ImGui::SeparatorText("请选择一个文件 ");
                    ImGui::PopStyleVar();
                    ImGui::EndChild();
                }
                ImGui::EndChild();
            }
            ImGui::SameLine();
            if (ImGui::BeginChild("##Output", ImVec2(0, 0), true)) {
                ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(1.0f, 0.5f));
                SeparatorTitle("输出 Output ", 1.75f);
                ImGui::PopStyleVar();
                ImGui::TextUnformatted("输出目录/文件:");
                ImGui::InputText("##输出", &outputFile);
                ImGui::SameLine();
                ChoosePath(outputFile);

                if (ImGui::Button(
                    "使用推荐值",
                    ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().x > 50.0f ? 50.0f : 0)
                ))
                    outputFile = inputFS.has_parent_path() && fs::is_regular_file(inputFS)
                                     ? inputFS.parent_path().string()
                                     : "Bin2C_Output";

                if (ImGui::BeginChild("###Path/File", ImVec2(), true)) {
                    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
                    if (fs::is_directory(outputFS)) {
                        ImGui::SeparatorText("目录信息:");

                        ImGui::TextWrapped("目录: %s", outputFS.filename().string().c_str());

                        ImGui::TextWrapped("路径:%s", "");
                        ImGui::SameLine();
                        ImGui::TextLinkOpenURL(outputFile.c_str(), outputFile.c_str());
                    } else if (fs::is_regular_file(outputFS)) {
                        ImGui::SeparatorText("文件信息:");

                        ImGui::TextWrapped("文件: %s", outputFS.has_stem() ? outputFS.filename().string().c_str() : "(空)");

                        ImGui::TextWrapped(
                            "类型: %s",
                            outputFS.has_extension() ? outputFS.extension().string().c_str() : "未知文件"
                        );

                        ImGui::TextWrapped("路径:%s", "");
                        ImGui::SameLine();
                        ImGui::TextLinkOpenURL(
                            inputFile.c_str(),
                            outputFS.has_parent_path()
                                ? outputFS.parent_path().string().c_str()
                                : outputFS.string().c_str()
                        );
                    } else ImGui::SeparatorText("请选择一个文件/目录");
                    ImGui::PopStyleVar();
                    ImGui::EndChild();
                }

                ImGui::EndChild();
            }
            ImGui::EndChild();
        }

        if (ImGui::BeginChild(
            "##Task",
            ImVec2(0, ImGui::GetContentRegionAvail().y - ImGui::CalcTextSize(BottomText).y - 5.0f),
            ImGuiChildFlags_Borders
        )) {
            ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));

            ImGui::Columns(3);

            const bool isNotFile = !fs::is_regular_file(inputFS);

            static bool hasErr = false;
            static std::string errMsg;

            if (isNotFile) {
                ImGui::SeparatorText("请选择一个输入文件");
                ImGui::BeginDisabled();
            } else ImGui::SeparatorText("二进制信息");

            if (ImGui::BeginChild("##二进制信息")) {
                if (!inputBin.DeclareOnly()) {
                    ImGui::TextWrapped("二进制大小: %zu", inputBin.GetSize());
                    ImGui::TextWrapped("二进制文件: %s", inputBin.GetFile().string().c_str());
                }
                ImGui::EndChild();
            }

            ImGui::NextColumn();

            if (isNotFile) {
                ImGui::SeparatorText("无可操作的文件");
            } else ImGui::SeparatorText("二进制操作");

            if (ImGui::BeginChild("##操作")) {
                static Bin2C::Output::Config cfg;
                try {
                    ImGui::Checkbox(" 美观化 ", &cfg.pretty);

                    ImGui::SameLine();
                    ImGui::Checkbox(" 常量化 ", &cfg.constant);

                    ImGui::SameLine();
                    ImGui::Checkbox(" 源输出 ", &cfg.source);

                    ImGui::Text("类型: ");
                    ImGui::SameLine();
                    static const char* 输出类型_选项[] = {
                            Bin2::Type::u8.GetName(),
                            Bin2::Type::u16.GetName(),
                            Bin2::Type::u32.GetName(),
                            Bin2::Type::u64.GetName()
                        };

                    static int 输出类型_SelectedItem = 0;
                    ImGui::Combo("##输出类型", &输出类型_SelectedItem, 输出类型_选项, IM_ARRAYSIZE(输出类型_选项));
                    switch (输出类型_SelectedItem) {
                        case 0:
                            cfg.flag = Bin2::Type::u8;
                            break;
                        case 1:
                            cfg.flag = Bin2::Type::u16;
                            break;
                        case 2:
                            cfg.flag = Bin2::Type::u32;
                            break;
                        case 3:
                            cfg.flag = Bin2::Type::u64;
                            break;
                        default:
                            输出类型_SelectedItem = 0;
                            break;
                    }

                    ImGui::BeginDisabled(cfg.source);
                    ImGui::Text("形式: ");
                    ImGui::SameLine();
                    static const char* 输出形式_选项[] = {
                            "默认 None",
                            "静态 static",
                            "内联 inline (C++17)"
                        };
                    static int 输出形式_SelectedItem = 0;
                    ImGui::Combo("##输出形式", &输出形式_SelectedItem, 输出形式_选项, IM_ARRAYSIZE(输出形式_选项));
                    if (输出形式_SelectedItem < 0 || 输出形式_SelectedItem >= IM_ARRAYSIZE(输出形式_选项))
                        输出形式_SelectedItem = 0;
                    cfg.exportType = static_cast<Bin2::ExportType>(输出形式_SelectedItem);
                    ImGui::EndDisabled();

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    const ImVec2 AContentRegionAvail(
                        ImGui::GetContentRegionAvail().x,
                        ImGui::GetContentRegionAvail().y / 2.5f
                    );

                    ImGui::BeginDisabled(inputBin.GetFile() == inputFS);
                    if (ImGui::Button(
                            std::format("加载二进制", inputFile).c_str(),
                            AContentRegionAvail
                        ) && fs::exists(inputFS)) {
                        inputBin(inputFS);
                    }
                    ImGui::EndDisabled();

                    ImGui::BeginDisabled(inputBin.DeclareOnly());
                    if (ImGui::Button(
                            std::format("导出二进制", outputFile).c_str(),
                            AContentRegionAvail
                        ) && fs::exists(outputFS)) {
                        if (inputBin.DeclareOnly()) {
                            hasErr = true;
                            errMsg = "二进制不可为空";
                        }
                        outputFuture = g_BackgroundTask.submit_task(
                            [&]() {
                                Bin2::Res result;
                                const Bin2C::Output functor = fs::is_regular_file(outputFS)
                                                                  ? Bin2C::Output(outputFS)
                                                                  : Bin2C::Output(outputFS, &inputBin, cfg);

                                return functor(&inputBin,cfg);
                            }
                        );
                    }
                    ImGui::EndDisabled();
                } catch (const std::exception& e) {
                    hasErr = true;
                    errMsg = e.what();
                }

                ImGui::EndChild();
            }

            ImGui::NextColumn();

            if (ImGui::BeginChild("##输出")) {
                if (outputFuture.valid()) {
                    g_OSS.clear();
                    if (auto [wasWrong, wrongMsg] = outputFuture.get(); !wasWrong)
                        g_OSS << "Error: " << wrongMsg << std::endl;
                    else g_OSS << "操作成功。" << std::endl;
                }
                ImGui::SeparatorText("输出");
                ImGui::TextWrapped("%s", g_OSS.str().c_str());
                ImGui::EndChild();
            }

            ImGui::Columns();

            if (isNotFile) ImGui::EndDisabled();

            if (hasErr) {
                ImGui::OpenPopup("##WarningPopup");
                if (ImGui::BeginPopupModal("##WarningPopup", &hasErr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    SeparatorTitle("警告 Warning", 1.5f);
                    ImGui::Spacing();
                    ImGui::TextWrapped("%s", errMsg.c_str());
                    ImGui::Separator();
                    ImGui::Text("请确保输入、输出信息设置正确或者权限是否足够");

                    ImGui::EndPopup();
                }
            }
            ImGui::PopStyleVar();
            ImGui::EndChild();
        }

        const float ContentWidth = ImGui::GetContentRegionAvail().x;
        ImGui::TextAligned(1.0f, ContentWidth - ImGui::CalcTextSize(TelegramChannel).x - 20.0f, "%s", BottomText);
        ImGui::SameLine();
        ImGui::SetCursorPosX(ContentWidth - ImGui::CalcTextSize(TelegramChannel).x);
        ImGui::TextLinkOpenURL(TelegramChannel, "https://ZCYMOD.T.ME");

        ImGui::End();
    }

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


int main(int /*argc*/, char** argv) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    g_PrimaryMonitor = glfwGetPrimaryMonitor();
    g_Scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());

    static const std::string AppName = fs::path(argv[0]).filename().string() + " | Bin2C-ImGui";
    g_Window = glfwCreateWindow(
        static_cast<int>(APP_WINDOW_DEFAULT_SIZE.x * g_Scale),
        static_cast<int>(APP_WINDOW_DEFAULT_SIZE.y * g_Scale),
        AppName.c_str(), nullptr, nullptr
    );

    if (!g_Window) return -1;

    glfwMakeContextCurrent(g_Window);
    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(g_Window)) {
        glfwPollEvents();
        if (glfwGetWindowAttrib(g_Window, GLFW_ICONIFIED) != 0) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }
        static ImVec2i display;
        glfwGetFramebufferSize(g_Window, &display.x, &display.y);
        glViewport(0, 0, display.x, display.y);
        glClearColor(
            g_BackgroundColor.x * g_BackgroundColor.w,
            g_BackgroundColor.y * g_BackgroundColor.w,
            g_BackgroundColor.z * g_BackgroundColor.w,
            g_BackgroundColor.w
        );
        glClear(GL_COLOR_BUFFER_BIT);
        GUI();
        glfwSwapBuffers(g_Window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    // ImGui::DestroyContext();
    // FIXME: 异常: Exception 0x80000003 encountered at address 0x7ffc21f06766

    glfwDestroyWindow(g_Window);
    glfwTerminate();

    return 0;
}
