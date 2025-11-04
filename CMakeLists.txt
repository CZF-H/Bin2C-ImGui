cmake_minimum_required(VERSION 4.0.2)
project(Bin2C-ImGui)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glfw3 REQUIRED)

include_directories(
	"third_party"
    "include"
)
link_directories(./libs)

file(GLOB IMGUI_SOURCES
	"third_party/imgui/*.cpp"
	"third_party/imgui/misc/cpp/*.cpp"

)
add_library(imgui STATIC
	${IMGUI_SOURCES}
	
	"third_party/imgui/backends/imgui_impl_glfw.cpp"
	"third_party/imgui/backends/imgui_impl_opengl3.cpp"
)
target_include_directories(imgui PUBLIC
	"third_party/imgui"
	"third_party/imgui/backends"
	"third_party/imgui/misc/cpp"
)

file(GLOB IMGUI_TOGGLE_SOURCES "third_party/imgui-toggle/*.cpp")
add_library(imgui-toggle STATIC ${IMGUI_TOGGLE_SOURCES})
target_include_directories(imgui-toggle PRIVATE
	"third_party/imgui"
)
target_link_libraries(imgui glfw)

add_library(NotoSansSC-Regular STATIC source/NotoSansSC-Regular.cpp)
add_library(ImGuiFileDialog STATIC third_party/ImGuiFileDialog/ImGuiFileDialog.cpp)
target_include_directories(ImGuiFileDialog PUBLIC
    "third_party/imgui"
    "third_party/ImGuiFileDialog"
)

add_executable(${CMAKE_PROJECT_NAME}
	main.cpp
 
)

#target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "-mwindows") # for mingw

target_link_libraries(${CMAKE_PROJECT_NAME}
	imgui
	imgui-toggle
    ImGuiFileDialog
    
    NotoSansSC-Regular
    
	glfw
	opengl32
)