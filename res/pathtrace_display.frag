#version 430

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 outColor;

uniform sampler2D texture0;
uniform float EXPOSURE;

vec3 acesFilm(vec3 color) {
    color *= EXPOSURE;
    return clamp((color * (2.51 * color + 0.03)) /
                 (color * (2.43 * color + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec3 linearColor = max(texture(texture0, fragTexCoord).rgb, vec3(0.0));
    outColor = vec4(pow(acesFilm(linearColor), vec3(1.0 / 2.2)), 1.0);
}
