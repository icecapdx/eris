"use strict";

const VERT_SRC = `#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;
uniform float uAngle;
out vec3 vColor;
void main() {
    float c = cos(uAngle), s = sin(uAngle);
    mat2 rot = mat2(c, -s, s, c);
    gl_Position = vec4(rot * aPos, 0.0, 1.0);
    vColor = aColor;
}
`;

const FRAG_SRC = `#version 330 core
in vec3 vColor;
uniform float uTime;
out vec4 fragColor;
void main() {
    // Pulse the brightness over time
    float pulse = 0.8 + 0.2 * sin(uTime * 2.0);
    fragColor = vec4(vColor * pulse, 1.0);
}
`;

const VERTICES = gl.createFloat32Array([
//   x      y     r     g     b
     0.0,  0.7,  1.0,  0.2,  0.2,   // top    — red
    -0.6, -0.5,  0.2,  1.0,  0.3,   // left   — green
     0.6, -0.5,  0.2,  0.3,  1.0,   // right  — blue
]);


function compileShader(type, source) {
    const shader = gl.createShader(type);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    return shader;
}

const vert = compileShader(gl.VERTEX_SHADER,   VERT_SRC);
const frag = compileShader(gl.FRAGMENT_SHADER, FRAG_SRC);

const prog = gl.createProgram();
gl.attachShader(prog, vert);
gl.attachShader(prog, frag);
gl.linkProgram(prog);
gl.deleteShader(vert);
gl.deleteShader(frag);

const uAngleLoc = gl.getUniformLocation(prog, "uAngle");
const uTimeLoc  = gl.getUniformLocation(prog, "uTime");

const vao = gl.createVertexArray();
gl.bindVertexArray(vao);

const vbo = gl.createBuffer();
gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
gl.bufferData(gl.ARRAY_BUFFER, VERTICES, gl.STATIC_DRAW);

const STRIDE = 5 * 4; // 5 floats × 4 bytes

gl.vertexAttribPointer(0, 2, gl.FLOAT, false, STRIDE, 0);       // aPos
gl.enableVertexAttribArray(0);

gl.vertexAttribPointer(1, 3, gl.FLOAT, false, STRIDE, 2 * 4);   // aColor
gl.enableVertexAttribArray(1);

gl.bindVertexArray(0);

let angle      = 0;
let spinSpeed  = 1.0;
let lastTime   = null;

function frame(timestamp) {
    if (lastTime === null) lastTime = timestamp;
    const dt = Math.min((timestamp - lastTime) / 1000, 0.1);
    lastTime = timestamp;

    if (input.isKeyDown("ArrowLeft"))  spinSpeed -= 2 * dt;
    if (input.isKeyDown("ArrowRight")) spinSpeed += 2 * dt;
    if (input.isKeyDown("Space"))      spinSpeed = 0;
    if (input.isKeyDown("Escape"))     quit();

    spinSpeed = Math.max(-10, Math.min(10, spinSpeed));
    angle += spinSpeed * dt;

    gl.clearColor(0.08, 0.08, 0.12, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT);

    gl.useProgram(prog);
    gl.uniform1f(uAngleLoc, angle);
    gl.uniform1f(uTimeLoc,  timestamp / 1000);

    gl.bindVertexArray(vao);
    gl.drawArrays(gl.TRIANGLES, 0, 3);
    gl.bindVertexArray(0);

    requestAnimationFrame(frame);
}

console.log(`Eris runtime — canvas: ${canvas.width}×${canvas.height}`);
console.log("← → Arrow keys: change spin speed  |  Space: stop  |  Esc: quit");

requestAnimationFrame(frame);