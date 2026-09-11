cmake_minimum_required(VERSION 3.28)

project(
    StayAwake
    VERSION 1.0.0
    DESCRIPTION "Keep Windows awake and request scheduled window closes"
    LANGUAGES CXX
)

include(CTest)

option(STAY_AWAKE_BUILD_APP "Build the application for a supported platform" ${WIN32})

function(stay_awake_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(
    stay_awake_core
    STATIC
        src/application/application.cpp
        src/application/durations.cpp
)
add_library(StayAwake::core ALIAS stay_awake_core)

target_compile_features(stay_awake_core PUBLIC cxx_std_17)
target_include_directories(
    stay_awake_core
    PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>"
)
set_target_properties(stay_awake_core PROPERTIES CXX_EXTENSIONS OFF)
stay_awake_enable_warnings(stay_awake_core)

if(STAY_AWAKE_BUILD_APP)
    if(WIN32)
        enable_language(RC)
        set(
            STAY_AWAKE_PLATFORM_SOURCES
            src/platform/windows/clock.cpp
            src/platform/windows/dpi.cpp
            src/platform/windows/main_window.cpp
            src/platform/windows/native.cpp
            src/platform/windows/overlay_window.cpp
            src/platform/windows/platform_factory.cpp
            src/platform/windows/power_manager.cpp
            src/platform/windows/single_instance.cpp
            src/platform/windows/tray_icon.cpp
            src/platform/windows/window_catalog.cpp
            src/platform/windows/windows_platform_binding.cpp
        )
        set(STAY_AWAKE_PLATFORM_LIBRARIES user32 gdi32 shell32 comctl32 mincore)
    else()
        message(FATAL_ERROR "No application platform binding is available for ${CMAKE_SYSTEM_NAME}")
    endif()

    add_library(stay_awake_platform STATIC ${STAY_AWAKE_PLATFORM_SOURCES})
    add_library(StayAwake::platform ALIAS stay_awake_platform)
    target_compile_features(stay_awake_platform PUBLIC cxx_std_17)
    target_compile_definitions(
        stay_awake_platform
        PRIVATE NOMINMAX UNICODE WIN32_LEAN_AND_MEAN _UNICODE _WIN32_WINNT=0x0A00 WINVER=0x0A00
    )
    target_include_directories(stay_awake_platform PRIVATE "${PROJECT_SOURCE_DIR}/src")
    target_link_libraries(
        stay_awake_platform
        PUBLIC StayAwake::core
        PRIVATE ${STAY_AWAKE_PLATFORM_LIBRARIES}
    )
    set_target_properties(stay_awake_platform PROPERTIES CXX_EXTENSIONS OFF)
    stay_awake_enable_warnings(stay_awake_platform)

    add_executable(stay_awake WIN32 src/main.cpp src/platform/windows/stay_awake.rc)
    target_include_directories(stay_awake PRIVATE "${PROJECT_SOURCE_DIR}/src/platform/windows")
    target_compile_definitions(stay_awake PRIVATE UNICODE _UNICODE)
    target_compile_features(stay_awake PRIVATE cxx_std_17)
    target_link_libraries(stay_awake PRIVATE StayAwake::platform)
    set_target_properties(stay_awake PROPERTIES CXX_EXTENSIONS OFF OUTPUT_NAME StayAwake)
    stay_awake_enable_warnings(stay_awake)

    if(MINGW)
        target_link_options(stay_awake PRIVATE -static -static-libgcc -static-libstdc++)
    endif()

    install(TARGETS stay_awake RUNTIME DESTINATION bin)
endif()

set(
    STAY_AWAKE_FORMAT_FILES
    "${PROJECT_SOURCE_DIR}/include/stay_awake/application.hpp"
    "${PROJECT_SOURCE_DIR}/include/stay_awake/application_state.hpp"
    "${PROJECT_SOURCE_DIR}/include/stay_awake/platform_binding.hpp"
    "${PROJECT_SOURCE_DIR}/include/stay_awake/platform_factory.hpp"
    "${PROJECT_SOURCE_DIR}/src/application/application.cpp"
    "${PROJECT_SOURCE_DIR}/src/application/durations.cpp"
    "${PROJECT_SOURCE_DIR}/src/main.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/clock.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/clock.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/dpi.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/dpi.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/main_window.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/main_window.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/native.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/native.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/overlay_window.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/overlay_window.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/platform_factory.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/power_manager.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/power_manager.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/resource.h"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/single_instance.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/single_instance.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/tray_icon.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/tray_icon.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/window_catalog.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/window_catalog.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/windows_platform_binding.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/windows_platform_binding.hpp"
    "${PROJECT_SOURCE_DIR}/tests/application/application.test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/application/durations.test.cpp"
)
include(cmake/ClangFormat.cmake)

if(BUILD_TESTING)
    include(FetchContent)

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/063de7e9578f82b369302001269680b4b1553359.tar.gz
        URL_HASH SHA256=623fe05d283020f37a579a8985ba338091ac7229da62b5591ff40acdf05b95e2
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(tests)
endif()
