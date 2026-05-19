cmake_minimum_required(VERSION 3.20)

# ─── Vita toolchain must be set BEFORE project() ─────────────────────────────
option(VITA_BUILD "Build for PlayStation Vita using vitaGL" OFF)

if(VITA_BUILD)
    if(NOT DEFINED ENV{VITASDK})
        message(FATAL_ERROR "VITASDK environment variable not set. "
            "Install VitaSDK from https://vitasdk.org and set VITASDK.")
    endif()
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake"
        CACHE PATH "Vita toolchain")
    set(VITA_APP_NAME  "Eris"       CACHE STRING "")
    set(VITA_TITLEID   "ERIS00001"  CACHE STRING "")
    set(VITA_VERSION   "01.00"      CACHE STRING "")
endif()

project(eris VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(BUILD_TESTING       OFF CACHE BOOL "" FORCE)
set(QUICKJS_BUILD_CLI   OFF CACHE BOOL "" FORCE)
set(CONFIG_LIBC         OFF CACHE BOOL "" FORCE)
add_subdirectory(framework/quickjs-ng)

if(VITA_BUILD)
    foreach(_t qjs-libc qjsc qjs_exe run-test262 function_source api-test lre-test)
        if(TARGET ${_t})
            set_target_properties(${_t} PROPERTIES EXCLUDE_FROM_ALL TRUE)
        endif()
    endforeach()
endif()


set(ERIS_SOURCES
    src/main.cpp
    src/runtime/runtime.cpp
    src/runtime/bindings/gl_bindings.cpp
    src/runtime/bindings/input_bindings.cpp
    src/runtime/bindings/audio_bindings.cpp
    src/runtime/bindings/canvas_bindings.cpp
    src/core/file_utils.cpp
)

if(NOT VITA_BUILD)
    find_package(SDL2       REQUIRED)
    find_package(SDL2_mixer REQUIRED)
    find_package(SDL2_ttf   REQUIRED)

    list(APPEND ERIS_SOURCES framework/glad/src/gl.c)

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
        SDL2_ttf::SDL2_ttf
        ${CMAKE_DL_LIBS}
    )

    foreach(dir js css sprites sounds)
        if(EXISTS ${CMAKE_SOURCE_DIR}/${dir})
            add_custom_command(TARGET eris POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${CMAKE_SOURCE_DIR}/${dir}
                    $<TARGET_FILE_DIR:eris>/${dir}
                COMMENT "Copying ${dir}/")
        endif()
    endforeach()
    if(EXISTS ${CMAKE_SOURCE_DIR}/entry.js)
        add_custom_command(TARGET eris POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/entry.js
                $<TARGET_FILE_DIR:eris>/entry.js
            COMMENT "Copying entry.js")
    endif()

else()
    include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

    add_executable(eris ${ERIS_SOURCES})

    target_compile_definitions(eris PRIVATE VITA_BUILD=1 ERIS_GLES=1)

    target_include_directories(eris PRIVATE
        src/
        framework/quickjs-ng/
        $ENV{VITASDK}/arm-vita-eabi/include
        $ENV{VITASDK}/arm-vita-eabi/include/SDL2
    )

    target_link_libraries(eris PRIVATE
        -Wl,--start-group
        qjs
        vitaGL
        vitashark
        SceShaccCgExt
        mathneon
        SDL2_mixer
        SDL2_ttf
        vorbisfile vorbis ogg
        opusfile opus
        modplug
        xmp
        SDL2
        SceDisplay_stub
        SceGxm_stub
        SceKernelDmacMgr_stub
        SceAudio_stub
        SceAudioIn_stub
        SceCtrl_stub
        SceTouch_stub
        SceHid_stub
        SceMotion_stub
        SceCommonDialog_stub
        SceIme_stub
        SceAppMgr_stub
        SceSysmodule_stub
        ScePower_stub
        SceFios2_stub
        SceVshBridge_stub
        SceShaccCg_stub
        taihen_stub
        SceLibKernel_stub
        SceKernelModulemgr_stub
        SceNet_stub
        SceNetCtl_stub
        SceRtc_stub
        SceSsl_stub
        pthread m z c
        -Wl,--end-group
    )

    vita_create_self(eris.self eris)

    vita_create_vpk(eris.vpk ${VITA_TITLEID} eris.self
        VERSION ${VITA_VERSION}
        NAME    ${VITA_APP_NAME}
    )

    message(STATUS "Vita VPK: ${VITA_TITLEID} — install js/ to ux0:data/eris/js/")
endif()
