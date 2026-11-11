#version 430 core
layout(location=0) in vec2  a_pos;
layout(location=1) in float a_size;
layout(location=2) in vec4  a_color;

out vec4 v_color;

uniform float u_zoom;
uniform vec2  u_pan;

void main() {
    vec2 pos     = (a_pos + u_pan) * u_zoom;
    gl_Position  = vec4(pos, 0.0, 1.0);
    gl_PointSize = a_size * u_zoom;
    v_color      = a_color;
}
