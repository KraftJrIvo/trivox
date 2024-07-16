#version 330

in vec2 fragCoord;
in vec4 fragColor;

out vec4 outColor;

uniform vec2 RESOLUTION;
uniform float TIME;

void main() {
    vec2 uv = fragCoord / RESOLUTION.xy;
    vec3 col = 0.5 + 0.5 * cos(TIME + uv.xyx + vec3(0,2,4));
    outColor = vec4(col, 1.0);
}