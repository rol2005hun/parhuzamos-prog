cmake_minimum_required(VERSION 3.20)
project(ParallelImageProcessor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Compiler flags ────────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/O2 /arch:AVX2 /openmp /W3)
else()
    # MinGW / GCC
    add_compile_options(-O2 -mavx2 -mfma -fopenmp -Wall -Wextra)
    add_link_options(-fopenmp)
endif()

# ── FetchContent ──────────────────────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.9
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# -- ImGui library (manual, no CMakeLists.txt in ImGui repo) -------------
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

# -- GLAD (FetchContent - glad CMake integration) -------------------------------
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v0.1.36
    GIT_SHALLOW    TRUE
)
set(GLAD_PROFILE "core"   CACHE STRING "" FORCE)
set(GLAD_API     "gl=4.3" CACHE STRING "" FORCE)
set(GLAD_GENERATOR "c"    CACHE STRING "" FORCE)
FetchContent_MakeAvailable(glad)

# -- Main application -------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/image/Image.cpp
    src/image/ContextFree.cpp
    src/image/ContextSensitive.cpp
    src/gpu/ComputeShader.cpp
    src/gpu/GLTexture.cpp
    src/ui/App.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    src/
    external/
    ${glad_SOURCE_DIR}/include
)

# OpenGL
find_package(OpenGL REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glad          # glad v0.1.36 FetchContent target
    glfw
    imgui
    OpenGL::GL
)

# gdi32, dwmapi needed on MinGW
if(MINGW)
    target_link_libraries(${PROJECT_NAME} PRIVATE gdi32 dwmapi)
endif()

# -- Copy shader files to build directory ------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/gpu/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMENT "Copying shaders..."
)

# -- Copy test image --------------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying assets..."
)
