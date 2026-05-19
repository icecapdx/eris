#pragma once
#include <quickjs.h>
#include <SDL.h>
#include <string>
#include <vector>

struct ErisRuntime {
    JSRuntime* rt  = nullptr;
    JSContext* ctx = nullptr;

    SDL_Window*   window  = nullptr;
    SDL_GLContext glctx   = nullptr;
    int           width   = 960;
    int           height  = 540;
    bool          running = true;

    const Uint8* keyboard = nullptr;
    int          mouseX   = 0;
    int          mouseY   = 0;
    Uint32       mouseBtn = 0;

    std::vector<JSValue> rafCallbacks;

    struct Interval {
        int     id;
        JSValue callback;
        double  intervalMs;
        double  nextFireMs;
    };
    std::vector<Interval> intervals;
    int nextIntervalId = 1;

    bool init(const std::string& entryScript);
    bool tick(double timestampMs);
    void shutdown();
};

void register_gl_bindings    (JSContext* ctx, ErisRuntime* rt);
void register_input_bindings (JSContext* ctx, ErisRuntime* rt);
void register_audio_bindings (JSContext* ctx, ErisRuntime* rt);
void register_canvas_bindings(JSContext* ctx, ErisRuntime* rt);
