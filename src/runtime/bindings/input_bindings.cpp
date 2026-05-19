#include <quickjs.h>
#include <SDL.h>
#include "runtime/runtime.h"

#ifdef VITA_BUILD
#  include <psp2/ctrl.h>
#  include <psp2/touch.h>

static SceCtrlData  g_pad     = {};
static SceTouchData g_touch   = {};
static bool         g_prevBtn = false;
static int          g_touchX  = 0;
static int          g_touchY  = 0;
static bool         g_touching = false;

static void vita_poll_input() {
    sceCtrlPeekBufferPositive(0, &g_pad, 1);
    sceTouchPeek(SCE_TOUCH_PORT_FRONT, &g_touch, 1);

    if (g_touch.reportNum > 0) {
        g_touchX   = (int)(g_touch.report[0].x * 960 / 1920);
        g_touchY   = (int)(g_touch.report[0].y * 544 / 1088);
        g_touching = true;
    } else {
        g_touching = false;
    }
}

static JSValue js_isKeyDown(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    if (argc < 1) return JS_FALSE;
    const char* key = JS_ToCString(ctx, argv[0]);
    if (!key) return JS_FALSE;

    unsigned int btn = 0;
    if      (strcmp(key, "ArrowUp")    == 0) btn = SCE_CTRL_UP;
    else if (strcmp(key, "ArrowDown")  == 0) btn = SCE_CTRL_DOWN;
    else if (strcmp(key, "ArrowLeft")  == 0) btn = SCE_CTRL_LEFT;
    else if (strcmp(key, "ArrowRight") == 0) btn = SCE_CTRL_RIGHT;
    else if (strcmp(key, "Enter")      == 0) btn = SCE_CTRL_CROSS;
    else if (strcmp(key, "Space")      == 0) btn = SCE_CTRL_CROSS;
    else if (strcmp(key, "Escape")     == 0) btn = SCE_CTRL_START;
    else if (strcmp(key, "KeyX")       == 0 || strcmp(key, "x") == 0) btn = SCE_CTRL_TRIANGLE;
    else if (strcmp(key, "KeyZ")       == 0 || strcmp(key, "z") == 0) btn = SCE_CTRL_SQUARE;
    else if (strcmp(key, "Shift")      == 0) btn = SCE_CTRL_LTRIGGER;

    JS_FreeCString(ctx, key);
    return JS_NewBool(ctx, btn ? (g_pad.buttons & btn) != 0 : 0);
}

static JSValue js_getMouseX(JSContext* ctx, JSValue, int, JSValue*) {
    return JS_NewInt32(ctx, g_touchX);
}
static JSValue js_getMouseY(JSContext* ctx, JSValue, int, JSValue*) {
    return JS_NewInt32(ctx, g_touchY);
}
static JSValue js_isMouseDown(JSContext* ctx, JSValue, int, JSValue*) {
    return JS_NewBool(ctx, g_touching);
}

void vita_update_input() { vita_poll_input(); }

#else
static SDL_Scancode scancode_from_js_key(const char* key) {
    if (!key) return SDL_SCANCODE_UNKNOWN;
    if (strcmp(key, "ArrowLeft")  == 0) return SDL_SCANCODE_LEFT;
    if (strcmp(key, "ArrowRight") == 0) return SDL_SCANCODE_RIGHT;
    if (strcmp(key, "ArrowUp")    == 0) return SDL_SCANCODE_UP;
    if (strcmp(key, "ArrowDown")  == 0) return SDL_SCANCODE_DOWN;
    if (strcmp(key, "Space")      == 0) return SDL_SCANCODE_SPACE;
    if (strcmp(key, "Enter")      == 0) return SDL_SCANCODE_RETURN;
    if (strcmp(key, "Escape")     == 0) return SDL_SCANCODE_ESCAPE;
    if (strcmp(key, "Shift")      == 0) return SDL_SCANCODE_LSHIFT;
    if (strcmp(key, "Control")    == 0) return SDL_SCANCODE_LCTRL;
    if (strcmp(key, "Alt")        == 0) return SDL_SCANCODE_LALT;
    if (strncmp(key, "Key", 3) == 0 && key[3]) {
        char c[2] = { (char)(key[3] | 32), 0 }; // lowercase
        return SDL_GetScancodeFromName(c);
    }
    if (strncmp(key, "Digit", 5) == 0 && key[5]) {
        char c[2] = { key[5], 0 };
        return SDL_GetScancodeFromName(c);
    }
    return SDL_GetScancodeFromName(key);
}

static JSValue js_isKeyDown(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    auto* eris = static_cast<ErisRuntime*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    if (!eris || !eris->keyboard || argc < 1) return JS_FALSE;

    const char* key = JS_ToCString(ctx, argv[0]);
    SDL_Scancode sc = scancode_from_js_key(key);
    JS_FreeCString(ctx, key);
    if (sc == SDL_SCANCODE_UNKNOWN) return JS_FALSE;

    return JS_NewBool(ctx, eris->keyboard[sc] != 0);
}

static JSValue js_getMouseX(JSContext* ctx, JSValue, int, JSValue*) {
    auto* eris = static_cast<ErisRuntime*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    return JS_NewInt32(ctx, eris ? eris->mouseX : 0);
}

static JSValue js_getMouseY(JSContext* ctx, JSValue, int, JSValue*) {
    auto* eris = static_cast<ErisRuntime*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    return JS_NewInt32(ctx, eris ? eris->mouseY : 0);
}

static JSValue js_isMouseDown(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    auto* eris = static_cast<ErisRuntime*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    if (!eris || argc < 1) return JS_FALSE;
    uint32_t btn = 0; JS_ToUint32(ctx, &btn, argv[0]);
    Uint32 sdlMask = (btn == 0) ? SDL_BUTTON_LMASK :
                     (btn == 1) ? SDL_BUTTON_MMASK : SDL_BUTTON_RMASK;
    return JS_NewBool(ctx, (eris->mouseBtn & sdlMask) != 0);
}
#endif

static JSValue js_quit(JSContext* ctx, JSValue, int, JSValue*) {
    auto* eris = static_cast<ErisRuntime*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    if (eris) eris->running = false;
    return JS_UNDEFINED;
}

void register_input_bindings(JSContext* ctx, ErisRuntime*) {
#ifdef VITA_BUILD
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
#endif

    JSValue input = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, input, "isKeyDown",
        JS_NewCFunction(ctx, js_isKeyDown,   "isKeyDown",   1));
    JS_SetPropertyStr(ctx, input, "getMouseX",
        JS_NewCFunction(ctx, js_getMouseX,   "getMouseX",   0));
    JS_SetPropertyStr(ctx, input, "getMouseY",
        JS_NewCFunction(ctx, js_getMouseY,   "getMouseY",   0));
    JS_SetPropertyStr(ctx, input, "isMouseDown",
        JS_NewCFunction(ctx, js_isMouseDown, "isMouseDown", 1));

    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "input", input);
    JS_SetPropertyStr(ctx, global, "quit",
        JS_NewCFunction(ctx, js_quit, "quit", 0));
    JS_FreeValue(ctx, global);
}
