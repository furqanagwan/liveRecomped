cmake_minimum_required(VERSION 3.25)

if(NOT TARGET rex::runtime)
    project(live_common LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(REXSDK_DIR "" CACHE PATH "rexglue-sdk source tree")
    if(REXSDK_DIR)
        add_subdirectory("${REXSDK_DIR}" rexglue-sdk)
    else()
        find_package(rexglue REQUIRED CONFIG)
    endif()
endif()

add_library(live_common STATIC
    src/app/game_paths.cpp
    src/app/live_recomp_app.cpp
    src/debug/guest_image_dump.cpp
    src/input/controller_menu_watcher.cpp
    src/input/guest_input_gate.cpp
    src/input/imgui_gamepad_bridge.cpp
    src/installer/disc_image_installer.cpp
    src/platform/gaming_runtime_session.cpp
    src/settings/user_settings_store.cpp
    src/ui/dialog_layout.cpp
    src/ui/disc_install_dialog.cpp
    src/ui/monochrome_theme.cpp
    src/ui/settings_dialog.cpp
    src/ui/system_menu_dialog.cpp
)

if(REXGLUE_PLATFORM_UWP)
    target_sources(live_common PRIVATE src/platform/native_file_picker_uwp.cpp)
    rexglue_uwp_cppwinrt_include_dir(live_cppwinrt_include_dir)
    target_include_directories(live_common SYSTEM PRIVATE "${live_cppwinrt_include_dir}")
elseif(WIN32)
    target_sources(live_common PRIVATE src/platform/native_file_picker_windows.cpp)
else()
    target_sources(live_common PRIVATE src/platform/native_file_picker_unix.cpp)
endif()

add_library(live::common ALIAS live_common)
target_compile_features(live_common PUBLIC cxx_std_23)
target_include_directories(live_common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(live_common PUBLIC rex::runtime)

if(TARGET imgui)
    target_include_directories(live_common PUBLIC $<TARGET_PROPERTY:imgui,INTERFACE_INCLUDE_DIRECTORIES>)
endif()

set(LIVE_GDK_ROOT "C:/Program Files (x86)/Microsoft GDK/260404/windows" CACHE PATH
    "Microsoft GDK windows folder containing include and lib/x64")

if(WIN32 AND NOT REXGLUE_PLATFORM_UWP AND EXISTS "${LIVE_GDK_ROOT}/include/XGameRuntimeInit.h")
    message(STATUS "live_common: Microsoft GDK found at ${LIVE_GDK_ROOT}")
    target_compile_definitions(live_common PRIVATE LIVE_HAS_GDK)
    target_include_directories(live_common PRIVATE "${LIVE_GDK_ROOT}/include")
    target_link_libraries(live_common PUBLIC "${LIVE_GDK_ROOT}/lib/x64/xgameruntime.lib" delayimp)
    target_link_options(live_common INTERFACE "LINKER:/DELAYLOAD:xgameruntime.dll")
endif()
