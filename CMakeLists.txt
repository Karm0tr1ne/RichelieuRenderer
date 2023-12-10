cmake_minimum_required(VERSION 3.10)

project(Richelieu)

set(CMAKE_CXX_STANDARD 17)

message(STATUS "Check VULKAN_SDK environment variable: $ENV{VULKAN_SDK}")
find_package(Vulkan REQUIRED)

add_subdirectory(third_party/glfw)
add_subdirectory(third_party/glm)

include_directories(include)
include_directories(third_party/imgui)
include_directories(third_party/imgui/backends)

add_executable(Richelieu
        src/VulkanApplicationBase.cpp
        src/VulkanTools.cpp
        src/VulkanDevice.cpp
        src/VulkanSwapchain.cpp
        src/VulkanModel.cpp
        src/VulkanBuffer.cpp
        src/VulkanTexture.cpp
        third_party/imgui/imgui.cpp
        third_party/imgui/imgui_draw.cpp
        third_party/imgui/imgui_tables.cpp
        third_party/imgui/imgui_widgets.cpp
        third_party/imgui/imgui_demo.cpp
        third_party/imgui/backends/imgui_impl_vulkan.cpp
        third_party/imgui/backends/imgui_impl_glfw.cpp
        examples/viking_room/viking_room.cpp
)

target_link_libraries(Richelieu glfw)
target_link_libraries(Richelieu glm)
target_link_libraries(Richelieu Vulkan::Vulkan)

