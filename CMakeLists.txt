cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message("compiling with debug flags")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,leak -fno-omit-frame-pointer -g -O2 -g -fno-omit-frame-pointer")
        set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address,leak")
    # endif()
endif()

project(video-thing VERSION 1.0.0)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/libs/imgui)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(NFD_FILE ${CMAKE_SOURCE_DIR}/libs/nativefiledialog/src/nfd_win.cpp)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(NFD_FILE ${CMAKE_SOURCE_DIR}/libs/nativefiledialog/src/nfd_cocoa.m)
else()
    message(compiling nfd for linux)
    set(NFD_FILE ${CMAKE_SOURCE_DIR}/libs/nativefiledialog/src/nfd_zenity.c)
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    ${NFD_FILE}
    ${CMAKE_SOURCE_DIR}/libs/nativefiledialog/src/nfd_common.c
    ${CMAKE_SOURCE_DIR}/libs/libyuv/source/*.cc
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)
# include_directories()
add_executable(${PROJECT_NAME} ${SOURCES})

include(cmake/CPM.cmake)

CPMAddPackage(
    NAME mlt
    GITHUB_REPOSITORY mltframework/mlt
    GIT_TAG v7.32.0
    OPTIONS
        "ENABLE_QT OFF"
        "ENABLE_QT6 OFF"
        "ENABLE_QT5 OFF"
        "ENABLE_AVFORMAT ON"
        "BUILD_TESTING OFF"
        "ENABLE_MLTPLUSPLUS ON"
        "RELOCATABLE OFF"
)
CPMAddPackage(
    NAME glad
    GITHUB_REPOSITORY Dav1dde/glad
    GIT_TAG v0.1.36 # latest stable
)
CPMAddPackage("gh:stephenberry/glaze@5.6.1")
CPMAddPackage("gh:libsdl-org/SDL#release-3.2.20")
CPMAddPackage("gh:mackron/miniaudio#0.11.23")

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavutil libavcodec libavformat libswscale libswresample
)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    libs/
    libs/imgui
    libs/libyuv/include
    libs/nativefiledialog/src/include
    libs/cereal/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    PkgConfig::FFMPEG
    SDL3::SDL3
    glad
    glaze::glaze
    miniaudio
    ${OpenCV_LIBS}
    mlt
    mlt++
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_BINARY_DIR}/out/lib/mlt
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources/mlt
)

