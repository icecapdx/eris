cmake_minimum_required(VERSION 3.20)
project(eris VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(framework/quickjs-ng)

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)

set(ERIS_SOURCES
    src/main.cpp
    src/runtime/runtime.cpp
    src/runtime/bindings/gl_bindings.cpp
    src/runtime/bindings/input_bindings.cpp
    src/runtime/bindings/audio_bindings.cpp
    src/runtime/bindings/canvas_bindings.cpp
    src/core/file_utils.cpp
    framework/glad/src/gl.c
)

add_executable(eris ${ERIS_SOURCES})

target_include_directories(eris PRIVATE
    src/
    framework/quickjs-ng/
    framework/glad/include/
)

target_link_libraries(eris PRIVATE
    qjs
    SDL2::SDL2
    SDL2_mixer::SDL2_mixer
    ${CMAKE_DL_LIBS}
)

add_custom_command(TARGET eris POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/js
        $<TARGET_FILE_DIR:eris>/js
    COMMENT "Copying JS game files"
)
