#include <quickjs.h>
#include <SDL_mixer.h>
#include "runtime/runtime.h"
#include <unordered_map>
#include <string>
#include <cstdio>

static std::unordered_map<int, Mix_Chunk*> g_chunks;
static std::unordered_map<int, Mix_Music*> g_music;
static int g_next_id = 1;

static JSValue js_loadSound(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    if (argc < 1) return JS_ThrowTypeError(ctx, "loadSound: needs path");
    const char* path = JS_ToCString(ctx, argv[0]);
    Mix_Chunk* chunk = Mix_LoadWAV(path);
    JS_FreeCString(ctx, path);
    if (!chunk) {
        fprintf(stderr, "[Audio] Mix_LoadWAV error: %s\n", Mix_GetError());
        return JS_NULL;
    }
    int id = g_next_id++;
    g_chunks[id] = chunk;
    return JS_NewInt32(ctx, id);
}

static JSValue js_playSound(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    if (argc < 1) return JS_UNDEFINED;
    int32_t id = 0; JS_ToInt32(ctx, &id, argv[0]);
    int loops = 0;
    if (argc >= 2) { int32_t l = 0; JS_ToInt32(ctx, &l, argv[1]); loops = l; }
    auto it = g_chunks.find(id);
    if (it != g_chunks.end())
        Mix_PlayChannel(-1, it->second, loops);
    return JS_UNDEFINED;
}

static JSValue js_loadMusic(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    if (argc < 1) return JS_ThrowTypeError(ctx, "loadMusic: needs path");
    const char* path = JS_ToCString(ctx, argv[0]);
    Mix_Music* music = Mix_LoadMUS(path);
    JS_FreeCString(ctx, path);
    if (!music) {
        fprintf(stderr, "[Audio] Mix_LoadMUS error: %s\n", Mix_GetError());
        return JS_NULL;
    }
    int id = g_next_id++;
    g_music[id] = music;
    return JS_NewInt32(ctx, id);
}

static JSValue js_playMusic(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    if (argc < 1) return JS_UNDEFINED;
    int32_t id = 0; JS_ToInt32(ctx, &id, argv[0]);
    int loops = -1;
    if (argc >= 2) { int32_t l = 0; JS_ToInt32(ctx, &l, argv[1]); loops = l; }
    auto it = g_music.find(id);
    if (it != g_music.end())
        Mix_PlayMusic(it->second, loops);
    return JS_UNDEFINED;
}

static JSValue js_stopMusic(JSContext*, JSValue, int, JSValue*) {
    Mix_HaltMusic();
    return JS_UNDEFINED;
}

static JSValue js_setSoundVolume(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    if (argc < 2) return JS_UNDEFINED;
    int32_t id = 0; JS_ToInt32(ctx, &id, argv[0]);
    double vol = 1.0; JS_ToFloat64(ctx, &vol, argv[1]);
    auto it = g_chunks.find(id);
    if (it != g_chunks.end())
        Mix_VolumeChunk(it->second, (int)(vol * MIX_MAX_VOLUME));
    return JS_UNDEFINED;
}

static JSValue js_setMusicVolume(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    if (argc < 1) return JS_UNDEFINED;
    double vol = 1.0; JS_ToFloat64(ctx, &vol, argv[0]);
    Mix_VolumeMusic((int)(vol * MIX_MAX_VOLUME));
    return JS_UNDEFINED;
}

void register_audio_bindings(JSContext* ctx, ErisRuntime* /*rt*/) {
    JSValue audio = JS_NewObject(ctx);

    JS_SetPropertyStr(ctx, audio, "loadSound",
        JS_NewCFunction(ctx, js_loadSound,      "loadSound",      1));
    JS_SetPropertyStr(ctx, audio, "play",
        JS_NewCFunction(ctx, js_playSound,      "play",           1));
    JS_SetPropertyStr(ctx, audio, "loadMusic",
        JS_NewCFunction(ctx, js_loadMusic,      "loadMusic",      1));
    JS_SetPropertyStr(ctx, audio, "playMusic",
        JS_NewCFunction(ctx, js_playMusic,      "playMusic",      1));
    JS_SetPropertyStr(ctx, audio, "stopMusic",
        JS_NewCFunction(ctx, js_stopMusic,      "stopMusic",      0));
    JS_SetPropertyStr(ctx, audio, "setSoundVolume",
        JS_NewCFunction(ctx, js_setSoundVolume, "setSoundVolume", 2));
    JS_SetPropertyStr(ctx, audio, "setMusicVolume",
        JS_NewCFunction(ctx, js_setMusicVolume, "setMusicVolume", 1));

    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "audio", audio);
    JS_FreeValue(ctx, global);
}
