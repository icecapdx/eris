#pragma once
#include <quickjs.h>
#include <SDL.h>
#include <string>
#include <functional>
#include <vector>

struct ErisRuntime {
    JSRuntime* rt   = nullptr;
    JSContext* ctx  = nullptr;

    SDL_Window*   window   = nullptr;
    SDL_GLContext glctx    = nullptr;
    int           width    = 1280;
    int           height   = 720;
    bool          running  = true;

    const Uint8* keyboard  = nullptr;
    int          mouseX    = 0;
    int          mouseY    = 0;
    Uint32       mouseBtn  = 0;

    std::vector<JSValue> rafCallbacks;

    bool init(const std::string& entryScript);

    bool tick(double timestampMs);

    void shutdown();
};

void register_gl_bindings    (JSContext* ctx, ErisRuntime* rt);
void register_input_bindings (JSContext* ctx, ErisRuntime* rt);
void register_audio_bindings (JSContext* ctx, ErisRuntime* rt);
void register_canvas_bindings(JSContext* ctx, ErisRuntime* rt);