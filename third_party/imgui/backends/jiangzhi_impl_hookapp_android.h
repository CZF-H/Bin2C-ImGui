// dear imgui: Platform Binding for Hooking Other Android native app
// This needs to be used along with the OpenGL 3 Renderer (imgui_impl_opengl3)

// Implemented features:
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy AKEYCODE_* values are obsolete since 1.87 and not supported since 1.91.5]
//  [X] Platform: Mouse support. Can discriminate Mouse/TouchScreen/Pen.
// Missing features or Issues:
//  [ ] Platform: Clipboard support.
//  [ ] Platform: Gamepad support.
//  [ ] Platform: Mouse cursor shape and visibility (ImGuiBackendFlags_HasMouseCursors). Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'. FIXME: Check if this is even possible with Android.
// Important:
//  - Consider using SDL or GLFW backend on Android, which will be more full-featured than this.
//  - FIXME: On-screen keyboard currently needs to be enabled by the application (see examples/ and issue #3446)
//  - FIXME: Unicode character inputs needs to be passed by Dear ImGui by the application (see examples/ and issue #3446)

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#pragma once

#include "imgui.h"      // IMGUI_IMPL_API

#ifndef IMGUI_DISABLE

struct ANativeWindow;
struct AInputEvent;

/**
 * @brief Follow "Getting Started" link and check examples/ folder to learn about using backends!
 * @param window <edit>可为nullptr（如使用HOOK）
 *
 * @warning Follow "Getting Started" link and check examples/ folder to learn about using backends!
 * @note 修改：window用于初始化渲染，当代码在已集成的环境运行可以为nullptr
 * @author 江芷酱紫
 */
IMGUI_IMPL_API bool ImGui_ImplAndroid_Init(ANativeWindow* window = nullptr);

/**
 * @param input_event
 * @param screen_scale <added>缩放因子
 *
 * @note 修改：可进行缩放（也可使用imgui示例的缩放，较为复杂，在OpenGL建议用此缩放）
 * @author 江芷酱紫
 */
IMGUI_IMPL_API int32_t ImGui_ImplAndroid_HandleInputEvent(const AInputEvent* input_event, const ImVec2& screen_scale = ImVec2(-1, -1));

IMGUI_IMPL_API void ImGui_ImplAndroid_Shutdown();

/**
 * @brief Follow "Getting Started" link and check examples/ folder to learn about using backends!
 * @param screen_width <add>可为空自动获取（如使用HOOK需要手动指定）
 * @param screen_height <add>可为空自动获取（如使用HOOK需要手动指定）
 *
 * @note 修改：对HOOK环境支持
 * @author 江芷酱紫
 */
IMGUI_IMPL_API void ImGui_ImplAndroid_NewFrame(int screen_width = -1, int screen_height = -1);

#endif // #ifndef IMGUI_DISABLE
