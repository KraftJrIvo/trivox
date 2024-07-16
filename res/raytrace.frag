#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 outColor;

uniform vec2 RESOLUTION;
uniform float TIME;

uniform mat4 CAM_MVP;
uniform float CAM_FOV;
uniform vec3 CAM_POS;

uniform vec3 LIGHT_POS = vec3(10);

struct Triangle {
    vec3 a;
    vec3 b;
    vec3 c;
    vec3 color;
};

struct Ray {
    vec3 pos;
    vec3 dir;
};

struct Intersection {
    bool ok;
    vec3 pos;
    vec3 normal;
    vec3 color;
};

const float EPS = 1e-6;

const uint N_TRIANGLES = uint(4);
//const vec3 offset = vec3(5.);
//const Triangle[N_TRIANGLES] TRIANGLES = Triangle[N_TRIANGLES]( 
//    Triangle(offset + vec3(0., 0., 0.), offset + vec3(0., 1., 0.), offset + vec3(1., 0., 0.), vec3(1., 0., 0.)), 
//    Triangle(offset + vec3(0., 0., 1.), offset + vec3(1., 0., 0.), offset + vec3(0., 1., 0.), vec3(0., 1., 0.)), 
//    Triangle(offset + vec3(0., 0., 1.), offset + vec3(0., 0., 0.), offset + vec3(1., 0., 0.), vec3(0., 0., 1.)), 
//    Triangle(offset + vec3(0., 0., 1.), offset + vec3(0., 1., 0.), offset + vec3(0., 0., 0.), vec3(1., 1., 0.))
//);

float packVec3(vec3 vec)
{
	int uValue;
	uValue = (int(vec.x / 10. * 65535.0 + 0.5));
	uValue |= (int(vec.y / 10. * 255.0 + 0.5)) << 16;
	uValue |= (int(vec.z / 10. * 253.0 + 1.5)) << 24;
	return float(uValue);
}

vec3 unpackVec3(float packedVec3)
{
	float a, b, c, d;
	int uInputFloat = int(packedVec3);
	a = ((uInputFloat) & 0xffff) / 65535.0;
	b = ((uInputFloat >> 16) & 0xff) / 255.0;
	c = (((uInputFloat >> 24) & 0xff) - 1.0) / 253.0;
	return vec3(a, b, c) * 10.;
}

vec3 packTriangle(Triangle t) {
    return vec3(packVec3(t.a), packVec3(t.b), packVec3(t.c));
}

Triangle unpackTriangle(vec3 pt, vec3 color) {
    return Triangle(unpackVec3(pt.x), unpackVec3(pt.y), unpackVec3(pt.z), color);
}

//const uint N_LVLS = 4
//uniform vec3[4] TRIANGLES;
//uniform int[5] TRIANG_INDICES = {4, 0, 1, 2, 3};

//const float[N_LVLS] LVL_R = {1. * 2. + .5, 2. * 2. + .5, 4. * 2. + .5, 8. * 2. + .5};
//uniform int[5 * 5 * 5 * N_LVLS] LVL_CELLS = {4, 0, 1, 2, 3};


//float Pack3PNForFP32(Vector3 channel)
//{
//	int uValue;
//	uValue = ((int)(channel.x * 65535.0f + 0.5f));
//	uValue |= ((int)(channel.y * 255.0f + 0.5f)) << 16;
//	uValue |= ((int)(channel.z * 253.0f + 1.5f)) << 24;
//	return (float)(uValue);
//}
//	
//Vector3 Unpack3PNFromFP32(float fFloatFromFP32)
//{
//	float a, b, c, d;
//	int uValue;
//	int uInputFloat = (int)(fFloatFromFP32);
//	a = ((uInputFloat) & 0xFFFF) / 65535.0f;
//	b = ((uInputFloat >> 16) & 0xFF) / 255.0f;
//	c = (((uInputFloat >> 24) & 0xFF) - 1.0f) / 253.0f;
//	return new Vector3(a, b, c);
//}

//vec3 getCurEntryPoint(vec3 absPos, float side, vec3 lastRes)
//{
//    float epsilon = constants.data[EPSILON];
//    vec3 result = vec3(0);
//    vec3 curPos = absPos / side;
//
//    for (int i = 0; i < 3; ++i)
//    {
//        while (curPos[i] < 0) curPos[i] += 1.0;
//        result[i] = (lastRes[i] < 0) ? 1.0 - epsilon : (curPos[i] - floor(curPos[i]));
//    }
//
//    return result;
//}
//
//vec3 marchAndGetNextDir(vec3 dir, float side, ivec2 minmax, uvec3 parals[8], inout bool finish, inout vec3 absPos, inout vec3 lastRes, inout vec3 absCoord)
//{
//    float epsilon = constants.data[EPSILON];
//
//    vec3 sgn = sign(dir + epsilon);
//    vec3 ispos = (sgn + 1.0) / 2.0;
//    vec3 isneg = 1.0 - ispos;
//
//    vec3 entry = getCurEntryPoint(absPos, side, lastRes);
//
//    vec3 vec = vec3(1.0, 2.0, 4.0) * (vec3(1,1,1) - isneg);
//    vec3 paral = parals[int(vec.x + vec.y + vec.z)];
//    if (side > 1) paral -= sign(floor(paral)) * (side - 1.0);
//
//    vec3 path = sgn * ((ispos - sgn * entry) * side + paral);
//    path = path + epsilon * (1.0 - sign(abs(path)));
//
//    vec3 diff = dir / path;
//    float maxDiff = max(diff.x, max(diff.y, diff.z));
//    vec3 resDiff = floor(diff / maxDiff + epsilon);
//    vec3 invResDiff = 1 - resDiff;
//    vec3 result = sgn * resDiff;
//
//    vec3 intersection = resDiff * (ispos * (side + paral) - isneg * paral - side * entry) + invResDiff * (length(resDiff * path) * dir / length(resDiff * dir));
//    absPos += intersection + sgn * epsilon;
//    absCoord += intersection / side;
//
//    finish = absPos[0] - epsilon <= minmax[0] || absPos[1] - epsilon <= minmax[0] || absPos[2] - epsilon <= minmax[0];
//
//    lastRes = result;
//    return result;
//}
//
//vec3 raytraceMap(vec3 rayStart, vec3 rayDir, inout vec3 normal, inout vec3 color)
//{
//    float chunkLoadRadius = constants.data[LOAD_RADIUS];
//    float chunkLoadDiameter = chunkLoadRadius * 2.0 + 1.0;
//
//    vec3 lastRes = vec3(0);
//    
//    ivec3 curChunkPos = ivec3(floor(rayStart));
//    bool notFinish = true;
//    bool marchFinish = false;
//    vec3 marchAbsPos = rayStart;
//
//    while (notFinish && !marchFinish)
//    {
//        if (checkMapBounds(curChunkPos))
//            break;
//
//        uint idx = uint((curChunkPos.x + chunkLoadRadius) * chunkLoadDiameter * chunkLoadDiameter + (curChunkPos.y + chunkLoadRadius) * chunkLoadDiameter + curChunkPos.z + chunkLoadRadius);
//        uint off = CHUNK_SIZE * idx;
//
//	    bool fullness;
//	    uint voxOff;
//	    uint side;
//	    uvec3 parals[8];
//        getChunkState(off, fullness, voxOff, side, parals);
//
//        vec3 prevHitPoint = marchAbsPos;
//        notFinish = !fullness || !raytraceChunk(fullness, voxOff, side, getCurEntryPoint(marchAbsPos, 1.0, lastRes), rayDir, curChunkPos, marchAbsPos, normal, color);
//
//        if (notFinish)
//        {
//            marchAbsPos = prevHitPoint;
//            vec3 coord;
//            marchAndGetNextDir(rayDir, 1, ivec2(-chunkLoadRadius, chunkLoadRadius + 1), parals, marchFinish, marchAbsPos, lastRes, coord);
//            curChunkPos = ivec3(floor(marchAbsPos));
//        }
//    }
//
//    if (marchFinish) {
//        marchAbsPos = clamp(rayDir * 1000000.0, vec3(-chunkLoadRadius), vec3(chunkLoadRadius + 1.0));
//    }
//
//    color *= vec3(max(1.0 - length(marchAbsPos - rayStart) / chunkLoadRadius, 0));
//
//    drawLights(rayStart, rayDir, marchAbsPos, color);
//
//    return marchAbsPos;
//}

//vec3 raytrace(vec3 rayStart, vec3 rayDir) {
//	vec3 abspos = abs(rayStart);
//	bool ok;
//	while (ok) {
//		float maxDist = max(max(abspos.x, abspos.y), abspos.z);
//		int minlvl = clamp(int(floor(log2(maxDist))), 0, 3);
//	}
//	//vec3 vmax = v * vec3(greaterThanEqual(vabs, vec3(m)));
//	//vec3 maxDistVec = max(max(max(absPos - LVL_R[0], absPos - LVL_R[1]), absPos - LVL_R[2]), absPos - LVL_R[3]);
//}

Intersection intersectTriangle(Ray ray, Triangle t) {
    Intersection inter;
	vec3 BA = t.b - t.a;
	vec3 CA = t.c - t.a;
	vec3 RDIRcrossCA = cross(ray.dir, CA);
	float det = dot(BA, RDIRcrossCA);
    vec3 RA = ray.pos - t.a;
    float baryU = dot(RA, RDIRcrossCA);
    vec3 RAcrossBA = cross(RA, BA);
    float baryV = dot(ray.dir, RAcrossBA);
    float dist = dot(CA, RAcrossBA);
    float invDet = 1.0 / det;
    inter.normal = cross(BA, CA);
    inter.pos = vec3(baryU * invDet, baryV * invDet, dist * invDet);
    inter.ok = (det >= EPS) && (baryU >= 0.0 && baryU <= det) && (baryV >= 0.0 && baryU + baryV <= det) && (dist > EPS);
    return inter;
}

Intersection raytrace_triangle(Ray ray, Triangle t) {
    Intersection inter = intersectTriangle(ray, t);
    if (inter.ok)
        inter.color = t.color;
    return inter;
}

vec3 raytrace_tmp(Ray ray) {
    vec3 offset = vec3(5. + sin(TIME) * 1.);
    Triangle[N_TRIANGLES] TRIANGLES = Triangle[N_TRIANGLES]( 
        Triangle(offset + vec3(0., 0., 0.), offset + vec3(0., 1., 0.), offset + vec3(1., 0., 0.), vec3(1., 0., 0.)), 
        Triangle(offset + vec3(0., 0., 1.), offset + vec3(1., 0., 0.), offset + vec3(0., 1., 0.), vec3(0., 1., 0.)), 
        Triangle(offset + vec3(0., 0., 1.), offset + vec3(0., 0., 0.), offset + vec3(1., 0., 0.), vec3(0., 0., 1.)), 
        Triangle(offset + vec3(0., 0., 1.), offset + vec3(0., 1., 0.), offset + vec3(0., 0., 0.), vec3(1., 1., 0.))
    );

    float bestDist = 1e10;
    Intersection bestInter = Intersection(false, vec3(0), vec3(0), vec3(0));
	for (uint i = uint(0); i < N_TRIANGLES; ++i) {
        //Triangle t = TRIANGLES[i];//
        Triangle t = unpackTriangle(packTriangle(TRIANGLES[i]), TRIANGLES[i].color);
        Intersection inter = raytrace_triangle(ray, t);
        if (inter.ok) {
            float dist = length(inter.pos - ray.pos);
            if (dist < bestDist) {
                bestDist = dist;
                bestInter = inter;
            }
        }
	}
    if (bestInter.ok) {
        vec3 lightDir = normalize(LIGHT_POS - bestInter.pos);
        return bestInter.color;// * dot(lightDir, bestInter.normal);
    }
    return vec3(0);
}

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

bool raytrace_sphere(vec3 orig, vec3 dir, vec3 center, float radius) {
    float t0, t1;
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
    vec3 start = CAM_POS;
    vec2 coeffs = (gl_FragCoord.xy - RESOLUTION / 2.0) / RESOLUTION.y;
    vec4 dirw = CAM_MVP * vec4(coeffs.x, coeffs.y, 1.0, 1.0);
    vec3 dir = normalize(dirw.xyz/dirw.w - start);
    vec3 col = raytrace_tmp(Ray(start, dir));
    if (raytrace_sphere(start, dir, vec3(0), 0.1) || raytrace_sphere(start, dir, vec3(2., 0., 0.), 0.1) || raytrace_sphere(start, dir, vec3(0., 0., 2.), 0.1) || raytrace_sphere(start, dir, vec3(0., 2., 0.), 0.1))
        col = vec3(1.0);
    outColor = vec4(col, 1.0);
}