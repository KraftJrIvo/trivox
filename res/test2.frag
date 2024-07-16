#version 330

in vec2 fragCoord;
in vec4 fragColor;

out vec4 outColor;

uniform vec2 RESOLUTION;
uniform float TIME;

uniform mat4 CAM_MVP;
uniform float CAM_FOV;
uniform vec3 CAM_POS;

uniform vec3 LIGHT_POS = vec3(0);

bool solveQuadratic(float a, float b, float c, out float x0, out float x1)
{
	float discr = b * b - 4 * a * c;

	if (discr < 0) 
        return false;

	else if (discr == 0) 
        x0 = x1 = -0.5 * b / a;
	else 
    {
		float q = (b > 0) ?
			-0.5 * (b + sqrt(discr)) :
			-0.5 * (b - sqrt(discr));
		x0 = q / a;
		x1 = c / q;
	}

	if (x0 > x1) 
    {
        float temp = x0;
        x0 = x1;
        x1 = temp;
    }

	return true;
}

bool raytrace_sphere(vec3 orig, vec3 dir, vec3 center, float radius)
{
    float t0, t1; // solutions for t if the ray intersects 

	float radius2 = radius * radius;

	vec3 L = orig - center;
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

	return true;
}

void main() {
    vec2 uv = fragCoord / RESOLUTION.xy;
    vec3 col = 0.5 + 0.5 * cos(TIME + uv.xyx + vec3(0,2,4));

    vec3 start = CAM_POS;

    vec2 coeffs = (gl_FragCoord.xy - RESOLUTION / 2.0) / RESOLUTION.y;
    vec4 dirw = CAM_MVP * vec4(coeffs.x, coeffs.y, -1.0, 1.0);
    vec3 dir = normalize(dirw.xyz/dirw.w - start);

    if (raytrace_sphere(start, dir, vec3(0), 1.0) || raytrace_sphere(start, dir, vec3(2., 0., 0.), 0.5) || raytrace_sphere(start, dir, vec3(0., 0., 2.), 0.25))
        col = vec3(0);

    outColor = vec4(col, 1.0);
}