#include <quickjs.h>
#include <SDL.h>
#include "runtime/runtime.h"
#include <cstdlib>
#include <ctime>

static JSValue js_canvas_get_width(JSContext* ctx, JSValue this_val) {
    auto* eris = static_cast<ErisRuntime*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    return JS_NewInt32(ctx, eris ? eris->width : 0);
}

static JSValue js_canvas_get_height(JSContext* ctx, JSValue this_val) {
    auto* eris = static_cast<ErisRuntime*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    return JS_NewInt32(ctx, eris ? eris->height : 0);
}

static JSValue js_performance_now(JSContext*, JSValue, int, JSValue*) {
    return JS_NewFloat64(nullptr,
        (double)SDL_GetTicks64() );
}

void register_canvas_bindings(JSContext* ctx, ErisRuntime* rt) {
    srand((unsigned)time(nullptr));

    JSValue canvas = JS_NewObject(ctx);

    JS_SetPropertyStr(ctx, canvas, "width",
        JS_NewInt32(ctx, rt->width));
    JS_SetPropertyStr(ctx, canvas, "height",
        JS_NewInt32(ctx, rt->height));

    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "canvas", canvas);

    JSValue perf = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, perf, "now",
        JS_NewCFunction(ctx, js_performance_now, "now", 0));
    JS_SetPropertyStr(ctx, global, "performance", perf);

    JS_SetPropertyStr(ctx, global, "window", JS_DupValue(ctx, global));

    JSValue doc = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, doc, "createElement",
        JS_NewCFunction(ctx, [](JSContext* c, JSValue, int, JSValue*) -> JSValue {
            return JS_NewObject(c);
        }, "createElement", 1));
    JS_SetPropertyStr(ctx, global, "document", doc);

    JS_FreeValue(ctx, global);
}
