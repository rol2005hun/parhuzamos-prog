#version 430 core
in vec4 v_color;
out vec4 fragColor;

void main() {
    // Kerek részecske renderelés Point sprite-ként
    vec2 coord = gl_PointCoord - vec2(0.5);
    float distSq = dot(coord, coord);
    if (distSq > 0.25) discard;
    
    // Lágy szél
    float alpha = (0.25 - distSq) * 4.0;
    fragColor = vec4(v_color.rgb, v_color.a * alpha);
}
