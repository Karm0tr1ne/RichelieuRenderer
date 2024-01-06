cmake_minimum_required(VERSION 3.10)

project(RichelieuRenderer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL 13)
    MESSAGE(STATUS "GCC13 detected disabling \"-Wdangling-reference\" in Cpp files as it appears to be a false positive")
    ADD_COMPILE_OPTIONS("$<$<COMPILE_LANGUAGE:CXX>:-Wno-dangling-reference>")
ENDIF()

message(STATUS "Check VULKAN_SDK environment variable: $ENV{VULKAN_SDK}")
find_package(Vulkan REQUIRED)

add_subdirectory(third_party/glfw)
add_subdirectory(third_party/glm)
add_subdirectory(third_party/assimp)

include_directories(include)
include_directories(third_party/imgui)
include_directories(third_party/imgui/backends)

file(GLOB BASE_SRC "src/*.cpp")

function(buildSingleExample EXAMPLE_NAME)
    set(EXAMPLE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/examples/${EXAMPLE_NAME})
    message(STATUS "Building example: ${EXAMPLE_NAME}")

    file(GLOB SOURCE *.cpp ${EXAMPLE_FOLDER}/*.cpp)
    set(MAIN_CPP ${EXAMPLE_FOLDER}/${EXAMPLE_NAME}.cpp)

    file(GLOB IMGUI_SOURCE "third_party/imgui/*.cpp" "third_party/imgui/backends/imgui_impl_vulkan.cpp" "third_party/imgui/backends/imgui_impl_glfw.cpp")
    set(SOURCE ${SOURCE} ${IMGUI_SOURCE})

    add_executable(${EXAMPLE_NAME} ${MAIN_CPP} ${SOURCE} ${IMGUI_SOURCE} ${BASE_SRC})
    target_link_libraries(${EXAMPLE_NAME} glfw)
    target_link_libraries(${EXAMPLE_NAME} glm)
    target_link_libraries(${EXAMPLE_NAME} assimp)
    target_link_libraries(${EXAMPLE_NAME} Vulkan::Vulkan)


endfunction(buildSingleExample)

function(buildAllExamples)
    foreach (EXAMPLE ${EXAMPLES})
        buildSingleExample(${EXAMPLE})
    endforeach ()
endfunction(buildAllExamples)

set(EXAMPLES
        PBR
        ray_tracing
        viking_room
)

buildAllExamples()