#ifdef VITA_BUILD
#  include <vitaGL.h>
#  include <psp2/kernel/processmgr.h>
#else
#  include <glad/gl.h>
#endif

#include <SDL.h>
#include <SDL_mixer.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "runtime/runtime.h"
#include "core/file_utils.h"

static bool create_window(ErisRuntime& eris, const char* title) {

#ifdef VITA_BUILD
    vglInitExtended(0, eris.width, eris.height, 0x800000, SCE_GXM_MULTISAMPLE_4X);

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    eris.window = SDL_CreateWindow(title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        eris.width, eris.height, SDL_WINDOW_SHOWN);
    eris.glctx = nullptr;

    printf("vitaGL initialised (%dx%d)\n", eris.width, eris.height);

#else
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    eris.window = SDL_CreateWindow(title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        eris.width, eris.height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    if (!eris.window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    eris.glctx = SDL_GL_CreateContext(eris.window);
    if (!eris.glctx) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        fprintf(stderr, "GLAD failed to load OpenGL function pointers\n");
        return false;
    }

    printf("OpenGL %s | %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));
#endif

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
        fprintf(stderr, "SDL_mixer warning: %s\n", Mix_GetError());

    return true;
}

static void present_frame(ErisRuntime& eris) {
#ifdef VITA_BUILD
    vglSwapBuffers(GL_FALSE);
#else
    SDL_GL_SwapWindow(eris.window);
#endif
}

int main(int argc, char* argv[]) {
#ifdef VITA_BUILD
    std::string entry = resolve_path("entry.js");
    (void)argc; (void)argv;
#else
    std::string entry = (argc >= 2) ? argv[1] : resolve_path("entry.js");
#endif

    ErisRuntime eris;
#ifdef VITA_BUILD
    eris.width  = 960;
    eris.height = 544;
#else
    eris.width  = 960;
    eris.height = 540;
#endif

    if (!create_window(eris, "Eris")) return 1;
    if (!eris.init(entry))            return 1;

    Uint64 perfFreq   = SDL_GetPerformanceFrequency();
    Uint64 startTicks = SDL_GetPerformanceCounter();

    SDL_Event event;

    while (eris.running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    eris.running = false;
                    break;
#ifndef VITA_BUILD
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        eris.width  = event.window.data1;
                        eris.height = event.window.data2;
                        glViewport(0, 0, eris.width, eris.height);
                    }
                    break;
#endif
                default: break;
            }
        }

        eris.keyboard = SDL_GetKeyboardState(nullptr);
        eris.mouseBtn = SDL_GetMouseState(&eris.mouseX, &eris.mouseY);

        double ts = (double)(SDL_GetPerformanceCounter() - startTicks)
                    / (double)perfFreq * 1000.0;

        if (!eris.tick(ts)) break;

        present_frame(eris);
    }

    eris.shutdown();

#if defined(VITA_BUILD) && defined(__vita__)
    //vglEnd();
    sceKernelExitProcess(0);
#endif
    return 0;
}
