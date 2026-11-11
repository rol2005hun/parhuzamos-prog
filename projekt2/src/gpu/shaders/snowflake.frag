#version 430 core
in float v_glow;
out vec4 fragColor;
uniform float u_time;
uniform vec3  u_color;

void main() {
    // Csillogó kék-fehér szín, idővel pulsálva
    float t    = u_time * 0.5;
    vec3  col  = mix(u_color, vec3(1.0), 0.3 + 0.2 * sin(t));
    fragColor  = vec4(col, 0.9);
}
