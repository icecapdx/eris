#include <quickjs.h>
#include <SDL.h>
#include "runtime/runtime.h"
#include "runtime/bindings/gl_platform.h"
#include <cstdlib>
#include <ctime>

static JSValue js_performance_now(JSContext*, JSValue, int, JSValue*) {
    return JS_NewFloat64(nullptr, (double)SDL_GetTicks64());
}

void register_canvas_bindings(JSContext* ctx, ErisRuntime* rt) {
    srand((unsigned)time(nullptr));

    JSValue global = JS_GetGlobalObject(ctx);

    JSValue canvas = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, canvas, "width",  JS_NewInt32(ctx, rt->width));
    JS_SetPropertyStr(ctx, canvas, "height", JS_NewInt32(ctx, rt->height));
    JS_SetPropertyStr(ctx, global, "canvas", canvas);
    JS_FreeValue(ctx, canvas);

    JSValue perf = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, perf, "now",
        JS_NewCFunction(ctx, js_performance_now, "now", 0));
    JS_SetPropertyStr(ctx, global, "performance", perf);
    JS_FreeValue(ctx, perf);

    JS_SetPropertyStr(ctx, global, "window", JS_DupValue(ctx, global));

    JSValue doc = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, doc, "createElement",
        JS_NewCFunction(ctx, [](JSContext* c, JSValue, int, JSValue*) -> JSValue {
            return JS_NewObject(c);
        }, "createElement", 1));
    JSValue body = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, body, "appendChild",
        JS_NewCFunction(ctx, [](JSContext* c, JSValue, int, JSValue*) -> JSValue {
            return JS_UNDEFINED;
        }, "appendChild", 1));
    JS_SetPropertyStr(ctx, doc, "body", body);
    JS_FreeValue(ctx, body);
    JS_SetPropertyStr(ctx, global, "document", doc);
    JS_FreeValue(ctx, doc);

    JS_FreeValue(ctx, global);
}
