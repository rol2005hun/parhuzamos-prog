#version 430 core
// Hópehely vonalak vertex shadere
layout(location=0) in vec2 a_pos;
uniform vec2  u_viewport; // pixel méret
uniform float u_time;

uniform float u_zoom;
uniform vec2  u_pan;

out float v_glow;

void main() {
    // Nincs pulsálás, statikus marad a vonal
    vec2 pos = (a_pos + u_pan) * u_zoom;
    gl_Position = vec4(pos, 0.0, 1.0);
    v_glow = 1.0;
}
