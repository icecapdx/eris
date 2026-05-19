#include "runtime/bindings/gl_platform.h"
#include <quickjs.h>
#include "runtime/runtime.h"
#include <cstdio>
#include <unordered_map>
#include <vector>
#include <string>

#define REQUIRE_ARGS(n) \
    if (argc < (n)) return JS_ThrowTypeError(ctx, "%s: needs %d args", __func__, (n))

#define ARG_INT32(v, i) ({ int32_t _v; JS_ToInt32(ctx, &_v, argv[(i)]); _v; })
#define ARG_UINT32(v, i) ({ uint32_t _v; JS_ToUint32(ctx, &_v, argv[(i)]); _v; })
#define ARG_FLOAT(v, i) ({ double _v; JS_ToFloat64(ctx, &_v, argv[(i)]); (float)_v; })
#define ARG_BOOL(v, i)  JS_ToBool(ctx, argv[(i)])

static JSValue wrap_handle(JSContext* ctx, GLuint id) {
    return JS_NewUint32(ctx, id);
}

static GLuint unwrap_handle(JSContext* ctx, JSValue v) {
    uint32_t id = 0;
    JS_ToUint32(ctx, &id, v);
    return (GLuint)id;
}

static JSValue js_createShader(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1);
    GLenum type = (GLenum)ARG_UINT32(, 0);
    return wrap_handle(ctx, glCreateShader(type));
}

static JSValue js_shaderSource(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(2);
    GLuint shader = unwrap_handle(ctx, argv[0]);
    const char* src = JS_ToCString(ctx, argv[1]);
    if (!src) return JS_EXCEPTION;
    glShaderSource(shader, 1, &src, nullptr);
    JS_FreeCString(ctx, src);
    return JS_UNDEFINED;
}

static JSValue js_compileShader(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1);
    GLuint shader = unwrap_handle(ctx, argv[0]);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(shader, 512, nullptr, log);
        fprintf(stderr, "[GL] Shader compile error: %s\n", log);
    }
    return JS_UNDEFINED;
}

static JSValue js_createProgram(JSContext* ctx, JSValue, int, JSValue*) {
    return wrap_handle(ctx, glCreateProgram());
}

static JSValue js_attachShader(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(2);
    glAttachShader(unwrap_handle(ctx, argv[0]), unwrap_handle(ctx, argv[1]));
    return JS_UNDEFINED;
}

static JSValue js_linkProgram(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1);
    GLuint prog = unwrap_handle(ctx, argv[0]);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetProgramInfoLog(prog, 512, nullptr, log);
        fprintf(stderr, "[GL] Program link error: %s\n", log);
    }
    return JS_UNDEFINED;
}

static JSValue js_useProgram(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1);
    glUseProgram(unwrap_handle(ctx, argv[0]));
    return JS_UNDEFINED;
}

static JSValue js_deleteShader(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1);
    glDeleteShader(unwrap_handle(ctx, argv[0]));
    return JS_UNDEFINED;
}

static JSValue js_getUniformLocation(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(2);
    GLuint prog = unwrap_handle(ctx, argv[0]);
    const char* name = JS_ToCString(ctx, argv[1]);
    if (!name) return JS_EXCEPTION;
    GLint loc = glGetUniformLocation(prog, name);
    JS_FreeCString(ctx, name);
    return JS_NewInt32(ctx, loc);
}

static JSValue js_getAttribLocation(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(2);
    GLuint prog = unwrap_handle(ctx, argv[0]);
    const char* name = JS_ToCString(ctx, argv[1]);
    if (!name) return JS_EXCEPTION;
    GLint loc = glGetAttribLocation(prog, name);
    JS_FreeCString(ctx, name);
    return JS_NewInt32(ctx, loc);
}

static JSValue js_uniform1f(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(2);
    glUniform1f(ARG_INT32(, 0), ARG_FLOAT(, 1));
    return JS_UNDEFINED;
}

static JSValue js_uniform2f(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(3);
    glUniform2f(ARG_INT32(, 0), ARG_FLOAT(, 1), ARG_FLOAT(, 2));
    return JS_UNDEFINED;
}

static JSValue js_uniform3f(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(4);
    glUniform3f(ARG_INT32(, 0), ARG_FLOAT(, 1), ARG_FLOAT(, 2), ARG_FLOAT(, 3));
    return JS_UNDEFINED;
}

static JSValue js_uniform4f(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(5);
    glUniform4f(ARG_INT32(, 0), ARG_FLOAT(, 1), ARG_FLOAT(, 2),
                ARG_FLOAT(, 3), ARG_FLOAT(, 4));
    return JS_UNDEFINED;
}

static JSValue js_uniform1i(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(2);
    glUniform1i(ARG_INT32(, 0), ARG_INT32(, 1));
    return JS_UNDEFINED;
}

static JSValue js_uniformMatrix4fv(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(3);
    GLint loc = ARG_INT32(, 0);
    GLboolean transp = (GLboolean)ARG_BOOL(, 1);

    size_t len; uint8_t* buf;
    buf = JS_GetArrayBuffer(ctx, &len, argv[2]);
    if (!buf) return JS_ThrowTypeError(ctx, "uniformMatrix4fv: expected ArrayBuffer");
    glUniformMatrix4fv(loc, (GLsizei)(len / (16 * sizeof(float))), transp,
                       reinterpret_cast<const float*>(buf));
    return JS_UNDEFINED;
}

static JSValue js_createBuffer(JSContext* ctx, JSValue, int, JSValue*) {
    GLuint buf; glGenBuffers(1, &buf);
    return wrap_handle(ctx, buf);
}

static JSValue js_bindBuffer(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(2);
    glBindBuffer((GLenum)ARG_UINT32(, 0), unwrap_handle(ctx, argv[1]));
    return JS_UNDEFINED;
}

static JSValue js_bufferData(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(3);
    GLenum target = (GLenum)ARG_UINT32(, 0);
    GLenum usage  = (GLenum)ARG_UINT32(, 2);

    size_t len; uint8_t* data;
    data = JS_GetArrayBuffer(ctx, &len, argv[1]);
    if (!data) return JS_ThrowTypeError(ctx, "bufferData: expected ArrayBuffer");
    glBufferData(target, (GLsizeiptr)len, data, usage);
    return JS_UNDEFINED;
}

static JSValue js_deleteBuffer(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1);
    GLuint b = unwrap_handle(ctx, argv[0]);
    glDeleteBuffers(1, &b);
    return JS_UNDEFINED;
}

static JSValue js_createVertexArray(JSContext* ctx, JSValue, int, JSValue*) {
    GLuint vao; glGenVertexArrays(1, &vao);
    return wrap_handle(ctx, vao);
}

static JSValue js_bindVertexArray(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1);
    glBindVertexArray(unwrap_handle(ctx, argv[0]));
    return JS_UNDEFINED;
}

static JSValue js_vertexAttribPointer(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(6);
    double stride_d, offset_d;
    JS_ToFloat64(ctx, &stride_d, argv[4]);
    JS_ToFloat64(ctx, &offset_d, argv[5]);
    glVertexAttribPointer(
        (GLuint)ARG_UINT32(, 0),
        ARG_INT32(, 1),
        (GLenum)ARG_UINT32(, 2),
        (GLboolean)ARG_BOOL(, 3),
        (GLsizei)stride_d,
        (const void*)(uintptr_t)offset_d
    );
    return JS_UNDEFINED;
}

static JSValue js_enableVertexAttribArray(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1);
    glEnableVertexAttribArray((GLuint)ARG_UINT32(, 0));
    return JS_UNDEFINED;
}

static JSValue js_createTexture(JSContext* ctx, JSValue, int, JSValue*) {
    GLuint tex; glGenTextures(1, &tex);
    return wrap_handle(ctx, tex);
}

static JSValue js_bindTexture(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(2);
    glBindTexture((GLenum)ARG_UINT32(, 0), unwrap_handle(ctx, argv[1]));
    return JS_UNDEFINED;
}

static JSValue js_texParameteri(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(3);
    glTexParameteri((GLenum)ARG_UINT32(, 0), (GLenum)ARG_UINT32(, 1), ARG_INT32(, 2));
    return JS_UNDEFINED;
}

static JSValue js_texImage2D(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(9);
    GLenum target  = (GLenum)ARG_UINT32(, 0);
    GLint  level   = ARG_INT32(, 1);
    GLint  ifmt    = ARG_INT32(, 2);
    GLsizei w      = ARG_INT32(, 3);
    GLsizei h      = ARG_INT32(, 4);
    GLint  border  = ARG_INT32(, 5);
    GLenum fmt     = (GLenum)ARG_UINT32(, 6);
    GLenum type    = (GLenum)ARG_UINT32(, 7);

    if (JS_IsNull(argv[8]) || JS_IsUndefined(argv[8])) {
        glTexImage2D(target, level, ifmt, w, h, border, fmt, type, nullptr);
    } else {
        size_t len; uint8_t* data = JS_GetArrayBuffer(ctx, &len, argv[8]);
        if (!data) return JS_ThrowTypeError(ctx, "texImage2D: expected ArrayBuffer or null");
        glTexImage2D(target, level, ifmt, w, h, border, fmt, type, data);
    }
    return JS_UNDEFINED;
}

static JSValue js_generateMipmap(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1);
    glGenerateMipmap((GLenum)ARG_UINT32(, 0));
    return JS_UNDEFINED;
}

static JSValue js_activeTexture(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1);
    glActiveTexture((GLenum)ARG_UINT32(, 0));
    return JS_UNDEFINED;
}

static JSValue js_clearColor(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(4);
    glClearColor(ARG_FLOAT(, 0), ARG_FLOAT(, 1), ARG_FLOAT(, 2), ARG_FLOAT(, 3));
    return JS_UNDEFINED;
}

static JSValue js_clear(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1);
    glClear((GLbitfield)ARG_UINT32(, 0));
    return JS_UNDEFINED;
}

static JSValue js_viewport(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(4);
    glViewport(ARG_INT32(, 0), ARG_INT32(, 1), ARG_INT32(, 2), ARG_INT32(, 3));
    return JS_UNDEFINED;
}

static JSValue js_drawArrays(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(3);
    glDrawArrays((GLenum)ARG_UINT32(, 0), ARG_INT32(, 1), ARG_INT32(, 2));
    return JS_UNDEFINED;
}

static JSValue js_drawElements(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(4);
    double offset; JS_ToFloat64(ctx, &offset, argv[3]);
    glDrawElements((GLenum)ARG_UINT32(, 0), ARG_INT32(, 1),
                   (GLenum)ARG_UINT32(, 2), (const void*)(uintptr_t)offset);
    return JS_UNDEFINED;
}

static JSValue js_enable(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1); glEnable((GLenum)ARG_UINT32(, 0)); return JS_UNDEFINED;
}
static JSValue js_disable(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1); glDisable((GLenum)ARG_UINT32(, 0)); return JS_UNDEFINED;
}
static JSValue js_blendFunc(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(2);
    glBlendFunc((GLenum)ARG_UINT32(, 0), (GLenum)ARG_UINT32(, 1));
    return JS_UNDEFINED;
}
static JSValue js_depthFunc(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1); glDepthFunc((GLenum)ARG_UINT32(, 0)); return JS_UNDEFINED;
}
static JSValue js_cullFace(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1); glCullFace((GLenum)ARG_UINT32(, 0)); return JS_UNDEFINED;
}

static JSValue js_createFramebuffer(JSContext* ctx, JSValue, int, JSValue*) {
    GLuint fb; glGenFramebuffers(1, &fb); return wrap_handle(ctx, fb);
}
static JSValue js_bindFramebuffer(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(2);
    glBindFramebuffer((GLenum)ARG_UINT32(, 0), unwrap_handle(ctx, argv[1]));
    return JS_UNDEFINED;
}
static JSValue js_framebufferTexture2D(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(5);
    glFramebufferTexture2D(
        (GLenum)ARG_UINT32(, 0), (GLenum)ARG_UINT32(, 1),
        (GLenum)ARG_UINT32(, 2), unwrap_handle(ctx, argv[3]),
        ARG_INT32(, 4)
    );
    return JS_UNDEFINED;
}
static JSValue js_checkFramebufferStatus(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1);
    return JS_NewUint32(ctx, glCheckFramebufferStatus((GLenum)ARG_UINT32(, 0)));
}

static void js_array_buffer_free(JSRuntime* rt, void* /*opaque*/, void* ptr) {
    js_free_rt(rt, ptr);
}

static JSValue js_createFloat32Array(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1);
    if (!JS_IsArray(argv[0]))
        return JS_ThrowTypeError(ctx, "createFloat32Array: expected JS Array");

    JSValue lenVal = JS_GetPropertyStr(ctx, argv[0], "length");
    int32_t len = 0; JS_ToInt32(ctx, &len, lenVal); JS_FreeValue(ctx, lenVal);
    if (len <= 0) return JS_ThrowRangeError(ctx, "createFloat32Array: empty array");

    float* fp = static_cast<float*>(js_malloc(ctx, (size_t)(len * 4)));
    if (!fp) return JS_EXCEPTION;

    for (int i = 0; i < len; i++) {
        JSValue el = JS_GetPropertyUint32(ctx, argv[0], (uint32_t)i);
        double d = 0; JS_ToFloat64(ctx, &d, el); JS_FreeValue(ctx, el);
        fp[i] = (float)d;
    }

    return JS_NewArrayBuffer(ctx, reinterpret_cast<uint8_t*>(fp),
                             (size_t)(len * 4), js_array_buffer_free, nullptr, false);
}

static JSValue js_createUint16Array(JSContext* ctx, JSValue, int argc, JSValue* argv) {
    REQUIRE_ARGS(1);
    if (!JS_IsArray(argv[0]))
        return JS_ThrowTypeError(ctx, "createUint16Array: expected JS Array");
    JSValue lenVal = JS_GetPropertyStr(ctx, argv[0], "length");
    int32_t len = 0; JS_ToInt32(ctx, &len, lenVal); JS_FreeValue(ctx, lenVal);
    if (len <= 0) return JS_ThrowRangeError(ctx, "createUint16Array: empty array");

    uint16_t* up = static_cast<uint16_t*>(js_malloc(ctx, (size_t)(len * 2)));
    if (!up) return JS_EXCEPTION;

    for (int i = 0; i < len; i++) {
        JSValue el = JS_GetPropertyUint32(ctx, argv[0], (uint32_t)i);
        uint32_t v = 0; JS_ToUint32(ctx, &v, el); JS_FreeValue(ctx, el);
        up[i] = (uint16_t)v;
    }

    return JS_NewArrayBuffer(ctx, reinterpret_cast<uint8_t*>(up),
                             (size_t)(len * 2), js_array_buffer_free, nullptr, false);
}

#define SET_FN(obj, name, fn, arity) \
    JS_SetPropertyStr(ctx, obj, name, JS_NewCFunction(ctx, fn, name, arity))

#define SET_CONST(obj, name, val) \
    JS_SetPropertyStr(ctx, obj, name, JS_NewUint32(ctx, val))

void register_gl_bindings(JSContext* ctx, ErisRuntime* /*rt*/) {
    JSValue gl = JS_NewObject(ctx);

    // Shader / program
    SET_FN(gl, "createShader",          js_createShader,          1);
    SET_FN(gl, "shaderSource",          js_shaderSource,          2);
    SET_FN(gl, "compileShader",         js_compileShader,         1);
    SET_FN(gl, "createProgram",         js_createProgram,         0);
    SET_FN(gl, "attachShader",          js_attachShader,          2);
    SET_FN(gl, "linkProgram",           js_linkProgram,           1);
    SET_FN(gl, "useProgram",            js_useProgram,            1);
    SET_FN(gl, "deleteShader",          js_deleteShader,          1);
    SET_FN(gl, "getUniformLocation",    js_getUniformLocation,    2);
    SET_FN(gl, "getAttribLocation",     js_getAttribLocation,     2);

    // Uniforms
    SET_FN(gl, "uniform1f",             js_uniform1f,             2);
    SET_FN(gl, "uniform2f",             js_uniform2f,             3);
    SET_FN(gl, "uniform3f",             js_uniform3f,             4);
    SET_FN(gl, "uniform4f",             js_uniform4f,             5);
    SET_FN(gl, "uniform1i",             js_uniform1i,             2);
    SET_FN(gl, "uniformMatrix4fv",      js_uniformMatrix4fv,      3);

    // Buffers
    SET_FN(gl, "createBuffer",          js_createBuffer,          0);
    SET_FN(gl, "bindBuffer",            js_bindBuffer,            2);
    SET_FN(gl, "bufferData",            js_bufferData,            3);
    SET_FN(gl, "deleteBuffer",          js_deleteBuffer,          1);

    // VAO
    SET_FN(gl, "createVertexArray",         js_createVertexArray,        0);
    SET_FN(gl, "bindVertexArray",           js_bindVertexArray,          1);
    SET_FN(gl, "vertexAttribPointer",       js_vertexAttribPointer,      6);
    SET_FN(gl, "enableVertexAttribArray",   js_enableVertexAttribArray,  1);

    // Textures
    SET_FN(gl, "createTexture",         js_createTexture,         0);
    SET_FN(gl, "bindTexture",           js_bindTexture,           2);
    SET_FN(gl, "texParameteri",         js_texParameteri,         3);
    SET_FN(gl, "texImage2D",            js_texImage2D,            9);
    SET_FN(gl, "generateMipmap",        js_generateMipmap,        1);
    SET_FN(gl, "activeTexture",         js_activeTexture,         1);

    // Draw / state
    SET_FN(gl, "clearColor",            js_clearColor,            4);
    SET_FN(gl, "clear",                 js_clear,                 1);
    SET_FN(gl, "viewport",              js_viewport,              4);
    SET_FN(gl, "drawArrays",            js_drawArrays,            3);
    SET_FN(gl, "drawElements",          js_drawElements,          4);
    SET_FN(gl, "enable",                js_enable,                1);
    SET_FN(gl, "disable",               js_disable,               1);
    SET_FN(gl, "blendFunc",             js_blendFunc,             2);
    SET_FN(gl, "depthFunc",             js_depthFunc,             1);
    SET_FN(gl, "cullFace",              js_cullFace,              1);

    // Framebuffers
    SET_FN(gl, "createFramebuffer",         js_createFramebuffer,        0);
    SET_FN(gl, "bindFramebuffer",           js_bindFramebuffer,          2);
    SET_FN(gl, "framebufferTexture2D",      js_framebufferTexture2D,     5);
    SET_FN(gl, "checkFramebufferStatus",    js_checkFramebufferStatus,   1);

    // ArrayBuffer helpers
    SET_FN(gl, "createFloat32Array",    js_createFloat32Array,    1);
    SET_FN(gl, "createUint16Array",     js_createUint16Array,     1);

    // WebGL constants
    SET_CONST(gl, "VERTEX_SHADER",              GL_VERTEX_SHADER);
    SET_CONST(gl, "FRAGMENT_SHADER",            GL_FRAGMENT_SHADER);
    SET_CONST(gl, "ARRAY_BUFFER",               GL_ARRAY_BUFFER);
    SET_CONST(gl, "ELEMENT_ARRAY_BUFFER",       GL_ELEMENT_ARRAY_BUFFER);
    SET_CONST(gl, "STATIC_DRAW",                GL_STATIC_DRAW);
    SET_CONST(gl, "DYNAMIC_DRAW",               GL_DYNAMIC_DRAW);
    SET_CONST(gl, "STREAM_DRAW",                GL_STREAM_DRAW);
    SET_CONST(gl, "FLOAT",                      GL_FLOAT);
    SET_CONST(gl, "UNSIGNED_BYTE",              GL_UNSIGNED_BYTE);
    SET_CONST(gl, "UNSIGNED_SHORT",             GL_UNSIGNED_SHORT);
    SET_CONST(gl, "UNSIGNED_INT",               GL_UNSIGNED_INT);
    SET_CONST(gl, "INT",                        GL_INT);
#ifndef __vita__
    SET_CONST(gl, "BOOL",                       GL_BOOL);
#endif
    SET_CONST(gl, "TRIANGLES",                  GL_TRIANGLES);
    SET_CONST(gl, "TRIANGLE_STRIP",             GL_TRIANGLE_STRIP);
    SET_CONST(gl, "LINES",                      GL_LINES);
    SET_CONST(gl, "POINTS",                     GL_POINTS);
    SET_CONST(gl, "COLOR_BUFFER_BIT",           GL_COLOR_BUFFER_BIT);
    SET_CONST(gl, "DEPTH_BUFFER_BIT",           GL_DEPTH_BUFFER_BIT);
    SET_CONST(gl, "STENCIL_BUFFER_BIT",         GL_STENCIL_BUFFER_BIT);
    SET_CONST(gl, "DEPTH_TEST",                 GL_DEPTH_TEST);
    SET_CONST(gl, "BLEND",                      GL_BLEND);
    SET_CONST(gl, "CULL_FACE",                  GL_CULL_FACE);
    SET_CONST(gl, "TEXTURE_2D",                 GL_TEXTURE_2D);
    SET_CONST(gl, "TEXTURE0",                   GL_TEXTURE0);
    SET_CONST(gl, "TEXTURE1",                   GL_TEXTURE1);
    SET_CONST(gl, "TEXTURE_MIN_FILTER",         GL_TEXTURE_MIN_FILTER);
    SET_CONST(gl, "TEXTURE_MAG_FILTER",         GL_TEXTURE_MAG_FILTER);
    SET_CONST(gl, "TEXTURE_WRAP_S",             GL_TEXTURE_WRAP_S);
    SET_CONST(gl, "TEXTURE_WRAP_T",             GL_TEXTURE_WRAP_T);
    SET_CONST(gl, "NEAREST",                    GL_NEAREST);
    SET_CONST(gl, "LINEAR",                     GL_LINEAR);
    SET_CONST(gl, "LINEAR_MIPMAP_LINEAR",       GL_LINEAR_MIPMAP_LINEAR);
    SET_CONST(gl, "REPEAT",                     GL_REPEAT);
    SET_CONST(gl, "CLAMP_TO_EDGE",              GL_CLAMP_TO_EDGE);
    SET_CONST(gl, "RGBA",                       GL_RGBA);
    SET_CONST(gl, "RGB",                        GL_RGB);
    SET_CONST(gl, "FRAMEBUFFER",                GL_FRAMEBUFFER);
    SET_CONST(gl, "COLOR_ATTACHMENT0",          GL_COLOR_ATTACHMENT0);
    SET_CONST(gl, "FRAMEBUFFER_COMPLETE",       GL_FRAMEBUFFER_COMPLETE);
    SET_CONST(gl, "SRC_ALPHA",                  GL_SRC_ALPHA);
    SET_CONST(gl, "ONE_MINUS_SRC_ALPHA",        GL_ONE_MINUS_SRC_ALPHA);
    SET_CONST(gl, "LESS",                       GL_LESS);
    SET_CONST(gl, "LEQUAL",                     GL_LEQUAL);
    SET_CONST(gl, "FRONT",                      GL_FRONT);
    SET_CONST(gl, "BACK",                       GL_BACK);

    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "gl", gl);
    JS_FreeValue(ctx, global);
}
