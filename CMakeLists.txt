cmake_minimum_required(VERSION 3.21)
project(DirectX12 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ImGui
include(FetchContent)

FetchContent_Declare(
        imgui_src
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui_src)

set(IMGUI_DIR ${imgui_src_SOURCE_DIR})

add_library(imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
        ${IMGUI_DIR}/backends/imgui_impl_dx12.cpp
)

target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC
        user32
        dxgi
        d3d12
        dxguid
        dwmapi
)

add_library(imgui::imgui ALIAS imgui)

# Application
add_executable(application WIN32
        include/utility/logger.h
        src/logger.cpp
        include/utility/timer.h
        src/timer.cpp
        include/framework/interface_framework.h
        include/framework/types.h
        src/interface_framework.cpp
        include/framework/exception/base_exception.h
        include/framework/exception/dx_exception.h
        include/framework/exception/win_exception.h
        include/framework/event/event_queue.h
        include/framework/event/events_window.h
        src/event_queue.cpp
        include/framework/windows_manager/windows_manager.h
        include/framework/windows_manager/inputs/keyboard.h
        include/framework/windows_manager/inputs/mouse.h
        src/mouse.cpp
        src/keyboard.cpp
        src/windows_manager.cpp
        include/framework/render_manager/render_manager.h
        src/render_manager.cpp
        include/application/application.h
        src/application.cpp
        src/main.cpp
        include/application/scene/interface_scene.h
        include/application/scene/scene_chapter_4.h
        src/scene_chapter_4.cpp
        include/application/registry_scene.h
        include/application/define_scenes.h
        include/utility/json_loader.h
        include/utility/file_system.h
        src/file_system.cpp
        include/utility/helpers.h
        src/helpers.cpp
        src/json_loader.cpp
        include/application/scene/scene_chapter_6.h
        src/scene_chapter_6.cpp
)

target_compile_definitions(application PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
)

target_include_directories(application PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(application PRIVATE
        imgui::imgui
        user32
        dxgi
        d3d12
        dxguid
        D3DCompiler
)

# Copy shaders
add_custom_command(TARGET application POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:application>/shaders"
)
