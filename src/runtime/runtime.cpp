#include "runtime.h"
#include "core/file_utils.h"

#ifdef VITA_BUILD
#  include <vitaGL.h>
#else
#  include <glad/gl.h>
#endif

#include <SDL.h>
#include <SDL_mixer.h>
#include <cstdio>
#include <stdexcept>

static void dump_js_exception(JSContext* ctx) {
    JSValue ex = JS_GetException(ctx);
    const char* msg = JS_ToCString(ctx, ex);
    fprintf(stderr, "[JS Exception] %s\n", msg ? msg : "(no message)");
    JS_FreeCString(ctx, msg);

    // Print stack if available
    JSValue stack = JS_GetPropertyStr(ctx, ex, "stack");
    if (!JS_IsUndefined(stack)) {
        const char* s = JS_ToCString(ctx, stack);
        if (s) fprintf(stderr, "%s\n", s);
        JS_FreeCString(ctx, s);
    }
    JS_FreeValue(ctx, stack);
    JS_FreeValue(ctx, ex);
}

static JSValue js_requestAnimationFrame(JSContext* ctx, JSValue,
                                        int argc, JSValue* argv,
                                        int, JSValue*) {
    auto* eris = static_cast<ErisRuntime*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    if (!eris) return JS_ThrowInternalError(ctx, "no ErisRuntime");
    if (argc < 1 || !JS_IsFunction(ctx, argv[0]))
        return JS_ThrowTypeError(ctx, "requestAnimationFrame expects a function");

    eris->rafCallbacks.push_back(JS_DupValue(ctx, argv[0]));
    return JS_NewInt32(ctx, (int)eris->rafCallbacks.size());
}

static JSValue js_console_log(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    for (int i = 0; i < argc; i++) {
        const char* s = JS_ToCString(ctx, argv[i]);
        if (s) { printf("%s%s", i ? " " : "", s); JS_FreeCString(ctx, s); }
    }
    printf("\n");
    return JS_UNDEFINED;
}

static JSValue js_console_error(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    for (int i = 0; i < argc; i++) {
        const char* s = JS_ToCString(ctx, argv[i]);
        if (s) { fprintf(stderr, "%s%s", i ? " " : "", s); JS_FreeCString(ctx, s); }
    }
    fprintf(stderr, "\n");
    return JS_UNDEFINED;
}

static JSValue js_include(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    if (argc < 1) return JS_ThrowTypeError(ctx, "include: needs a path");
    const char* raw = JS_ToCString(ctx, argv[0]);
    if (!raw) return JS_EXCEPTION;
    std::string path = resolve_path(raw);
    JS_FreeCString(ctx, raw);

    std::string src = read_file(path);
    if (src.empty()) {
        fprintf(stderr, "[include] Cannot read: %s\n", path.c_str());
        return JS_ThrowReferenceError(ctx, "include: file not found");
    }
    JSValue result = JS_Eval(ctx, src.c_str(), src.size(),
                             path.c_str(), JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(result)) return result;
    JS_FreeValue(ctx, result);
    return JS_UNDEFINED;
}

static JSValue js_setTimeout(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    auto* eris = static_cast<ErisRuntime*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    if (!eris || argc < 1) return JS_NewInt32(ctx, -1);
    if (!JS_IsFunction(ctx, argv[0]))
        return JS_ThrowTypeError(ctx, "setTimeout: arg 0 must be a function");

    double delayMs = 0;
    if (argc >= 2) JS_ToFloat64(ctx, &delayMs, argv[1]);

    ErisRuntime::Interval iv;
    iv.id         = eris->nextIntervalId++;
    iv.callback   = JS_DupValue(ctx, argv[0]);
    iv.intervalMs = -1;
    iv.nextFireMs = (double)SDL_GetTicks64() + delayMs;
    eris->intervals.push_back(iv);
    return JS_NewInt32(ctx, iv.id);
}

static JSValue js_setInterval(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    auto* eris = static_cast<ErisRuntime*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    if (!eris || argc < 2) return JS_NewInt32(ctx, -1);
    if (!JS_IsFunction(ctx, argv[0]))
        return JS_ThrowTypeError(ctx, "setInterval: arg 0 must be a function");

    double delayMs = 16.0;
    JS_ToFloat64(ctx, &delayMs, argv[1]);
    if (delayMs < 1) delayMs = 1;

    ErisRuntime::Interval iv;
    iv.id         = eris->nextIntervalId++;
    iv.callback   = JS_DupValue(ctx, argv[0]);
    iv.intervalMs = delayMs;
    iv.nextFireMs = (double)SDL_GetTicks64() + delayMs;
    eris->intervals.push_back(iv);
    return JS_NewInt32(ctx, iv.id);
}

static JSValue js_clearTimer(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    auto* eris = static_cast<ErisRuntime*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    if (!eris || argc < 1) return JS_UNDEFINED;
    int32_t id = 0; JS_ToInt32(ctx, &id, argv[0]);
    for (auto it = eris->intervals.begin(); it != eris->intervals.end(); ++it) {
        if (it->id == id) {
            JS_FreeValue(ctx, it->callback);
            eris->intervals.erase(it);
            break;
        }
    }
    return JS_UNDEFINED;
}

bool ErisRuntime::init(const std::string& entryScript) {
    rt  = JS_NewRuntime();
    ctx = JS_NewContext(rt);
    if (!rt || !ctx) { fprintf(stderr, "Failed to create QuickJS runtime\n"); return false; }

    JS_SetMemoryLimit(rt, 128 * 1024 * 1024); // 128 MB heap
    JS_SetRuntimeOpaque(rt, this);

    JSValue global = JS_GetGlobalObject(ctx);

    JSValue console = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, console, "log",
        JS_NewCFunction(ctx, js_console_log,   "log",   1));
    JS_SetPropertyStr(ctx, console, "error",
        JS_NewCFunction(ctx, js_console_error, "error", 1));
    JS_SetPropertyStr(ctx, console, "warn",
        JS_NewCFunction(ctx, js_console_log,   "warn",  1));
    JS_SetPropertyStr(ctx, global, "console", console);
    JS_FreeValue(ctx, console);

    JSValue raf = JS_NewCFunctionData(ctx, js_requestAnimationFrame,
                                      1, 0, 0, nullptr);
    JS_SetPropertyStr(ctx, global, "requestAnimationFrame", raf);

    JS_SetPropertyStr(ctx, global, "include",
        JS_NewCFunction(ctx, js_include, "include", 1));

    JS_SetPropertyStr(ctx, global, "setTimeout",
        JS_NewCFunction(ctx, js_setTimeout,  "setTimeout",  2));
    JS_SetPropertyStr(ctx, global, "setInterval",
        JS_NewCFunction(ctx, js_setInterval, "setInterval", 2));
    JS_SetPropertyStr(ctx, global, "clearTimeout",
        JS_NewCFunction(ctx, js_clearTimer,  "clearTimeout",  1));
    JS_SetPropertyStr(ctx, global, "clearInterval",
        JS_NewCFunction(ctx, js_clearTimer,  "clearInterval", 1));

    JS_FreeValue(ctx, global);

    register_canvas_bindings(ctx, this);
    register_gl_bindings    (ctx, this);
    register_input_bindings (ctx, this);
    register_audio_bindings (ctx, this);

    std::string resolved = resolve_path(entryScript);
    std::string src = read_file(resolved);
    if (src.empty()) {
        fprintf(stderr, "Could not read entry script: %s\n", resolved.c_str());
        return false;
    }

    JSValue result = JS_Eval(ctx, src.c_str(), src.size(),
                             resolved.c_str(), JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(result)) {
        dump_js_exception(ctx);
        JS_FreeValue(ctx, result);
        return false;
    }
    JS_FreeValue(ctx, result);
    return true;
}

bool ErisRuntime::tick(double timestampMs) {
    JSContext* pending_ctx;
    for (;;) {
        int r = JS_ExecutePendingJob(rt, &pending_ctx);
        if (r == 0) break;
        if (r < 0) { dump_js_exception(pending_ctx); break; }
    }

    double nowMs = (double)SDL_GetTicks64();
    std::vector<int> ready;
    for (auto& iv : intervals)
        if (nowMs >= iv.nextFireMs) ready.push_back(iv.id);

    for (int firedId : ready) {
        int idx = -1;
        for (int j = 0; j < (int)intervals.size(); j++)
            if (intervals[j].id == firedId) { idx = j; break; }
        if (idx < 0) continue;

        JSValue cb         = JS_DupValue(ctx, intervals[idx].callback);
        double  intervalMs = intervals[idx].intervalMs;

        JSValue ret = JS_Call(ctx, cb, JS_UNDEFINED, 0, nullptr);
        if (JS_IsException(ret)) dump_js_exception(ctx);
        JS_FreeValue(ctx, ret);
        JS_FreeValue(ctx, cb);

        idx = -1;
        for (int j = 0; j < (int)intervals.size(); j++)
            if (intervals[j].id == firedId) { idx = j; break; }
        if (idx < 0) continue;

        if (intervalMs < 0) {
            JS_FreeValue(ctx, intervals[idx].callback);
            intervals.erase(intervals.begin() + idx);
        } else {
            intervals[idx].nextFireMs += intervalMs;
        }
    }

    std::vector<JSValue> cbs;
    cbs.swap(rafCallbacks);

    JSValue tsVal = JS_NewFloat64(ctx, timestampMs);
    for (JSValue& cb : cbs) {
        JSValue ret = JS_Call(ctx, cb, JS_UNDEFINED, 1, &tsVal);
        if (JS_IsException(ret)) dump_js_exception(ctx);
        JS_FreeValue(ctx, ret);
        JS_FreeValue(ctx, cb);
    }
    JS_FreeValue(ctx, tsVal);

    return running;
}

void ErisRuntime::shutdown() {
    for (JSValue& v : rafCallbacks) JS_FreeValue(ctx, v);
    rafCallbacks.clear();
    for (auto& iv : intervals) JS_FreeValue(ctx, iv.callback);
    intervals.clear();

    if (ctx) { JS_FreeContext(ctx); ctx = nullptr; }
    if (rt)  { JS_FreeRuntime(rt);  rt  = nullptr; }

    Mix_CloseAudio();
    Mix_Quit();

#ifndef VITA_BUILD
    if (glctx)  { SDL_GL_DeleteContext(glctx); glctx = nullptr; }
#endif
    if (window) { SDL_DestroyWindow(window);   window = nullptr; }
    SDL_Quit();
}
