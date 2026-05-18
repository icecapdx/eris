#include "runtime.h"
#include "core/file_utils.h"
#include <glad/gl.h>
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

static JSValue js_requestAnimationFrame(JSContext* ctx, JSValue /*this_val*/,
                                        int argc, JSValue* argv,
                                        int /*magic*/, JSValue* /*func_data*/) {
    auto* eris = static_cast<ErisRuntime*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    if (!eris) return JS_ThrowInternalError(ctx, "no ErisRuntime");
    if (argc < 1 || !JS_IsFunction(ctx, argv[0]))
        return JS_ThrowTypeError(ctx, "requestAnimationFrame expects a function");

    eris->rafCallbacks.push_back(JS_DupValue(ctx, argv[0]));
    return JS_NewInt32(ctx, (int)eris->rafCallbacks.size());
}

static JSValue js_console_log(JSContext* ctx, JSValue /*this*/, int argc, JSValue* argv) {
    for (int i = 0; i < argc; i++) {
        const char* s = JS_ToCString(ctx, argv[i]);
        if (s) { printf("%s%s", i ? " " : "", s); JS_FreeCString(ctx, s); }
    }
    printf("\n");
    return JS_UNDEFINED;
}

static JSValue js_console_error(JSContext* ctx, JSValue /*this*/, int argc, JSValue* argv) {
    for (int i = 0; i < argc; i++) {
        const char* s = JS_ToCString(ctx, argv[i]);
        if (s) { fprintf(stderr, "%s%s", i ? " " : "", s); JS_FreeCString(ctx, s); }
    }
    fprintf(stderr, "\n");
    return JS_UNDEFINED;
}

bool ErisRuntime::init(const std::string& entryScript) {
    rt  = JS_NewRuntime();
    ctx = JS_NewContext(rt);
    if (!rt || !ctx) { fprintf(stderr, "Failed to create QuickJS runtime\n"); return false; }

    JS_SetMemoryLimit(rt, 128 * 1024 * 1024); // 128 MB heap

    JSValue global = JS_GetGlobalObject(ctx);

    JSValue console = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, console, "log",
        JS_NewCFunction(ctx, js_console_log,   "log",   1));
    JS_SetPropertyStr(ctx, console, "error",
        JS_NewCFunction(ctx, js_console_error, "error", 1));
    JS_SetPropertyStr(ctx, global, "console", console);

    JS_SetRuntimeOpaque(rt, this);
    JSValue raf = JS_NewCFunctionData(ctx, js_requestAnimationFrame,
                                      1 /*length*/, 0 /*magic*/, 0 /*data_len*/, nullptr);
    JS_SetPropertyStr(ctx, global, "requestAnimationFrame", raf);

    JS_FreeValue(ctx, global);

    register_canvas_bindings(ctx, this);
    register_gl_bindings    (ctx, this);
    register_input_bindings (ctx, this);
    register_audio_bindings (ctx, this);

    std::string src = read_file(entryScript);
    if (src.empty()) {
        fprintf(stderr, "Could not read entry script: %s\n", entryScript.c_str());
        return false;
    }

    JSValue result = JS_Eval(ctx, src.c_str(), src.size(),
                             entryScript.c_str(), JS_EVAL_TYPE_GLOBAL);
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

    if (ctx) { JS_FreeContext(ctx); ctx = nullptr; }
    if (rt)  { JS_FreeRuntime(rt);  rt  = nullptr; }

    Mix_CloseAudio();
    Mix_Quit();

    if (glctx)  { SDL_GL_DeleteContext(glctx); glctx = nullptr; }
    if (window) { SDL_DestroyWindow(window);   window = nullptr; }
    SDL_Quit();
}