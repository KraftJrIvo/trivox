#version 430

const uint TRIVOX_MAX_ROOMS = 16;

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 outColor;

struct Room {
    uvec3 size;
    vec3 cellSz;
    uint firstCellIdx;
};

struct RoomRef {
    mat4 matrix;
    vec3 color;
    uint idx;
};

layout(std140, binding = 0) 
buffer Rooms
{
    Room rooms_data[];
};
layout (std140, binding = 1) 
buffer RoomRefs {
    RoomRef roomrefs_data[];
};

uniform vec2 RESOLUTION;
uniform float TIME;

uniform mat4 CAM_MVP;
uniform float CAM_FOV;
uniform vec3 CAM_POS;

uniform sampler2D texture0;
uniform sampler2D texture1;

struct Sphere {
    vec3 center;
    float radius;
    vec3 color;
};

struct Ray {
    vec3 pos;
    vec3 dir;
};

bool solveQuadratic(float a, float b, float c, out float x0, out float x1) {
	float discr = b * b - 4 * a * c;
	if (discr < 0) 
        return false;
	else if (discr == 0) 
        x0 = x1 = -0.5 * b / a;
	else {
		float q = (b > 0) ?
			-0.5 * (b + sqrt(discr)) :
			-0.5 * (b - sqrt(discr));
		x0 = q / a;
		x1 = c / q;
	}
	if (x0 > x1) {
        float temp = x0;
        x0 = x1;
        x1 = temp;
    }
	return true;
}

bool raytrace_sphere(vec3 orig, vec3 dir, Sphere sphere, inout vec3 col) {
    float t0, t1;
	float radius2 = sphere.radius * sphere.radius;
	vec3 L = orig - sphere.center;
	float a = dot(dir, dir);
	float b = 2 * dot(dir, L);
	float c = dot(L, L) - radius2;
	if (!solveQuadratic(a, b, c, t0, t1)) 
        return false;
	if (t0 > t1) {
        float temp = t0;
        t0 = t1;
        t1 = t0;
    }
	if (t0 < 0) {
		t0 = t1;
		if (t0 < 0) 
            return false;
	}
    col = sphere.color;
	return true;
}

void main() {
    vec3 start = CAM_POS;
    vec2 coeffs = (gl_FragCoord.xy - RESOLUTION * .5) / (RESOLUTION.y * .5);
    vec4 dirw = CAM_MVP * vec4(coeffs.x, coeffs.y, 1.0, 1.0);
    vec3 dir = normalize(dirw.xyz/dirw.w - start);
    vec3 col = texture(texture0, fragTexCoord).rgb;
    bool intersect = false;

    for (int i = 0; i < TRIVOX_MAX_ROOMS; ++i) {
        RoomRef rr = roomrefs_data[i];
        if (rr.idx > 0) {
            Room r = rooms_data[0];
            vec3 origin = rr.matrix[3].xyz;
            vec3 center = origin + mat3(rr.matrix) * (vec3(r.size) * .5);
            Sphere sphere = Sphere(center, .5, rr.color);
            if (raytrace_sphere(start, dir, sphere, col)) {
                intersect = true;
                break;
            }
        }
    }

    vec3 frontCol = texture(texture1, fragTexCoord).rgb;
    if (length(frontCol) == 0)
        outColor = vec4(col, 1.0);
    else
        outColor = vec4(frontCol, 1.0);
}