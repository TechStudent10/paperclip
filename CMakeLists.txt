cmake_minimum_required(VERSION 3.21...4.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS OFF)

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

if(APPLE)
    enable_language(OBJC)
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/libs)

include(cmake/CPM.cmake)

set(NFD_PORTAL ON)
CPMAddPackage("gh:geode-sdk/json@3.2.1")
CPMAddPackage("gh:libsdl-org/SDL#release-3.2.20")
# CPMAddPackage("gh:USCiLab/cereal#a56bad8")
CPMAddPackage("gh:geode-sdk/result@1.3.2")
CPMAddPackage("gh:mackron/miniaudio#0.11.23")
CPMAddPackage("gh:btzy/nativefiledialog-extended#6db343a")
CPMAddPackage("gh:lemenkov/libyuv#5b22f31")
CPMAddPackage("gh:nothings/stb#fede005")
CPMAddPackage("gh:fmtlib/fmt#12.0.0")
CPMAddPackage("gh:g-truc/glm#4962d27")
CPMAddPackage("gh:freetype/freetype#4334f00")

CPMAddPackage(
    NAME imgui
    GITHUB_REPOSITORY ocornut/imgui
    GIT_TAG 8de97d1
    DOWNLOAD_ONLY ON
)
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${imgui_SOURCE_DIR}/misc/cpp ${json_SOURCE_DIR}/include)
target_link_libraries(imgui PUBLIC SDL3::SDL3 OpenGL::GL)

add_library(glad STATIC
    libs/glad/src/gl.c
)
target_include_directories(glad PUBLIC libs/glad/include)

if (WIN32)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/out/share/mlt)

    # set(WIN_ICONV_BUILD_EXECUTABLE OFF)
    # set(WIN_ICONV_BUILD_SHARED OFF)
    # CPMAddPackage("gh:win-iconv/win-iconv#82f00fb")
    # set(Iconv_LIBRARY iconv_static)
    # set(Iconv_INCLUDE_DIR ${win-iconv_SOURCE_DIR})

    # CPMAddPackage("gh:dlfcn-win32/dlfcn-win32#8bfddb5")
    # set(CMAKE_DL_LIBS dl)

    # CPMAddPackage("gh:GerHobbelt/pthread-win32#version-3.1.0-release")
    # set(Threads_LIBRARY ${pthreads-win32-2-9-1_SOURCE_DIR}/x64/pthreadVC2.lib)
    # set(Threads_INCLUDE_DIR ${pthreads-win32-2-9-1_SOURCE_DIR})
    # include_directories(${pthreads-win32-2-9-1_SOURCE_DIR})
    # add_definitions(-DHAVE_STRUCT_TIMESPEC)
    # add_definitions(-D_USE_MATH_DEFINES)

    # some hacky mlt fixes im sure i won't regret later
    add_compile_definitions(stat64i32=_stati64)
    add_compile_definitions(_TIME_BITS=64)
    # add_compile_definitions(pthread_cond_timedwait64=pthread_cond_timedwait)
endif ()

CPMAddPackage(
    NAME mlt
    GITHUB_REPOSITORY TechStudent10/mlt
    GIT_TAG sans-nanosleep
    OPTIONS
        "MLT_DATA_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib/mlt"
        "MOD_QT OFF"
        "MOD_MOVIT ON"
        "ENABLE_QT OFF"
        "ENABLE_QT6 OFF"
        "ENABLE_QT5 OFF"
        "ENABLE_AVFORMAT ON"
        "ENABLE_MLTPLUSPLUS ON"
        "BUILD_TESTING OFF"
        "RELOCATABLE OFF"
)

# add_compile_definitions(mlt PUBLIC WIN_PTHREADS_TIME_H=1)

if(WIN32)
    set_target_properties(mlt PROPERTIES EXCLUDE_FROM_ALL TRUE)
    add_library(_mlt SHARED IMPORTED)
    set_target_properties(_mlt PROPERTIES
        IMPORTED_LOCATION "libs/mlt/libmlt-7.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${mlt_SOURCE_DIR}/src"
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE _mlt)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE mlt)
endif()

set(FFMPEG_ROOT "libs/ffmpeg")

if(WIN32)
    set(FFMPEG_LIB_DIR "${FFMPEG_ROOT}/ext/windows")
elseif(APPLE)
    set(FFMPEG_LIB_DIR "${FFMPEG_ROOT}/ext/mac")
else()
    set(FFMPEG_LIB_DIR "${FFMPEG_ROOT}/ext/linux")
endif()

include_directories(${FFMPEG_ROOT}/include)
link_directories(${FFMPEG_LIB_DIR})

find_package(OpenGL REQUIRED)
# find_package(OpenCV REQUIRED)
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
#     libavutil libavcodec libavformat libswscale libswresample
# )

target_link_libraries(${PROJECT_NAME} PRIVATE
    # PkgConfig::FFMPEG
    OpenGL::GL
    # ${OpenCV_LIBS}
    yuv
    # cereal
    imgui
    nfd
    SDL3::SDL3
    mat-json
    miniaudio
    # mlt++
    GeodeResult
    fmt
    glad
    glm::glm
    freetype
)

if (APPLE)
    find_library(AVCODEC_LIBRARY avcodec)
    find_library(AVFORMAT_LIBRARY avformat)
    find_library(AVUTIL_LIBRARY avutil)
    find_library(SWSCALE_LIBRARY swscale)
    find_library(SWRESAMPLE_LIBRARY swresample)

    target_link_libraries(${PROJECT_NAME} PRIVATE ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY} ${SWRESAMPLE_LIBRARY})
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE avutil avcodec avformat swscale swresample)
endif()

# add_dependencies(${PROJECT_NAME} mltavformat)

target_include_directories(${PROJECT_NAME} PRIVATE ${stb_SOURCE_DIR} ${libyuv_SOURCE_DIR}/include)

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

