#version 430

const float EPS = 0.001;
const uint TRIVOX_MAX_ROOMS = 16;
const float POS_INF = 1. / 0.;
const float NEG_INF = -1. / 0.;
const float MIN_VSZ = 100.;
const int MIN_LVL = 0;
const int MAX_LVL = 3;
const float FOG_DIST = 100.;
const vec3 FOG_COLOR = vec3(0.);

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 outColor;

struct Room {
    vec3 sz;
    uint firstCellIdx;
};

struct RoomRef {
    mat4 mat;
    vec3 col;
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
    vec3 o;
    float r;
};

struct Box {
    vec3 o;
    mat3 rot;
    vec3 sz;    
};

struct AAC {
    vec3 o;
    float side;
};

struct Ray {
    vec3 o;
    vec3 dir;
};

struct Intersection {
    bool exists;
    vec3 o;
    vec3 n;
    vec3 col;
    Ray exit;
};

Intersection raytrace_sphere(Ray ray, Sphere sphere) {
    vec3 oc = ray.o - sphere.o;
    float a = dot(ray.dir, ray.dir);
    float b = 2.0 * dot(oc, ray.dir);
    float c = dot(oc, oc) - sphere.r * sphere.r;
    float discriminant = b * b - 4.0 * a * c;
    
    Intersection result;
    result.exists = false;
    
    if (discriminant >= 0.0) {
        float t = (-b - sqrt(discriminant)) / (2.0 * a);
        if (t > 0.0) {
            result.exists = true;
            result.o = ray.o + t * ray.dir;
            result.n = normalize(result.o - sphere.o);
            result.col = result.n;
        }
    }
    
    return result;
}

Intersection raytrace_box(Ray ray, Box box) {
    Intersection result;
    result.exists = false;
    Ray lray = Ray((ray.o - box.o) * box.rot, ray.dir * box.rot);
    vec3 tMin = -lray.o / lray.dir;
    vec3 tMax = (box.sz - lray.o) / lray.dir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    bool inside = all(greaterThanEqual(lray.o, vec3(0.))) && all(lessThanEqual(lray.o, box.sz));
    if (inside || (tNear <= tFar && tNear >= 0.)) {
        result.exists = true;
        vec3 localPos = lray.o + lray.dir * tNear;
        result.o = box.rot * localPos + box.o;
        vec3 locn = vec3(0.0);
        float epsilon = 0.001;
        if (abs(localPos.x) < epsilon) locn.x = -1.;
        else if (abs(localPos.x - box.sz.x) < epsilon) locn.x = 1.;
        else if (abs(localPos.y) < epsilon) locn.y = -1.;
        else if (abs(localPos.y - box.sz.y) < epsilon) locn.y = 1.;
        else if (abs(localPos.z) < epsilon) locn.z = -1.;
        else if (abs(localPos.z - box.sz.z) < epsilon) locn.z = 1.;
        result.n = box.rot * locn;
        //result.col = localPos * .1;
        vec3 localExit = lray.o + lray.dir * tFar;
        result.exit = Ray(box.rot * localExit + box.o, ray.dir);
        if (inside)
            result.o = ray.o;
    }
    return result;
}

Intersection raytrace_aac(Ray ray, AAC aac) {
    vec3 tmin = (aac.o - ray.o) / ray.dir;
    vec3 tmax = (aac.o + vec3(aac.side) - ray.o) / ray.dir;
    vec3 t1 = min(tmin, tmax);
    vec3 t2 = max(tmin, tmax);
    float t_near = max(max(t1.x, t1.y), t1.z);
    float t_far = min(min(t2.x, t2.y), t2.z);
    Intersection result;
    result.exists = false;
    bool inside = (ray.o.x >= aac.o.x && ray.o.x <= aac.o.x + aac.side &&
                ray.o.y >= aac.o.y && ray.o.y <= aac.o.y + aac.side &&
                ray.o.z >= aac.o.z && ray.o.z <= aac.o.z + aac.side);
    if (inside || (t_near <= t_far && t_near > 0.0)) {
        result.exists = true;
        result.o = ray.o + t_near * ray.dir;
        vec3 pos = result.o - aac.o;
        vec3 half_side = vec3(aac.side * 0.5);
        vec3 n = vec3(0.0);
        if (abs(pos.x) < 0.001) n = vec3(-1.0, 0.0, 0.0);
        else if (abs(pos.x - aac.side) < 0.001) n = vec3(1.0, 0.0, 0.0);
        else if (abs(pos.y) < 0.001) n = vec3(0.0, -1.0, 0.0);
        else if (abs(pos.y - aac.side) < 0.001) n = vec3(0.0, 1.0, 0.0);
        else if (abs(pos.z) < 0.001) n = vec3(0.0, 0.0, -1.0);
        else if (abs(pos.z - aac.side) < 0.001) n = vec3(0.0, 0.0, 1.0);
        result.n = n;
        result.col = vec3(1.0);
        result.exit = Ray(ray.o + t_far * ray.dir, ray.dir);
    }
    
    return result;
}

vec3 minIndicator(vec3 v) {
    float minVal = min(min(v.x, v.y), v.z);
    return vec3(
        step(minVal, v.x) * step(v.x, minVal),
        step(minVal, v.y) * step(v.y, minVal),
        step(minVal, v.z) * step(v.z, minVal)
    );
}

Intersection raytrace_room(Ray ray, RoomRef rr, Box box) {
    Intersection res;
    res.exists = false;

    vec3 lcampos = (CAM_POS - box.o) * box.rot;

    Room r = rooms_data[rr.idx - 1];
    Ray lray = Ray(clamp((ray.o - box.o) * box.rot, vec3(EPS), r.sz - EPS), ray.dir * box.rot);
    bool inside = all(greaterThanEqual(lray.o, vec3(0.))) && all(lessThanEqual(lray.o, box.sz));

    bool first = true;

    while (inside) {
        float lvl, csz, vsz;
        vec3 cellc;
        for (int i = MAX_LVL; i >= MIN_LVL; --i) {
            csz = pow(2, i);
            cellc = floor(lray.o / csz) * csz + vec3(.5) * csz;
            vsz = (2. * atan((csz * .5) / length(cellc - lcampos))) * RESOLUTION.y;
            if (i == MIN_LVL || vsz < MIN_VSZ) {
                lvl = float(i);
                break;
            }
        }

        //vec3 cellc = floor(lray.o) + vec3(.5);
        //float vsz = (2. * atan(1. / length(cellc - lcampos))) * RESOLUTION.y;
        //float lvl = clamp(floor(log2(MIN_VSZ / vsz)), 0., 3.);
        //float csz = pow(2, lvl);

        AAC aac = AAC(floor(lray.o / csz) * csz, csz);
        Sphere sph = Sphere(aac.o + vec3(csz * .5), csz * .45);
        res = raytrace_sphere(lray, sph);
        if (res.exists) {
            res.o = box.rot * res.o + box.o;
            return res;
        } else {
            res = raytrace_aac(lray, aac);
            lray.o = res.exit.o + res.exit.dir * EPS;
            inside = all(greaterThanEqual(lray.o, vec3(0.))) && all(lessThanEqual(lray.o, box.sz));
            res.exists = false;
        }
    }

    return res;
}

Intersection raytrace_rooms(Ray ray) 
{
    vec3 start = ray.o;
    bool first = true;
    bool hit = false;
    float path = 0.;
    Intersection last;
    Intersection closest;
    uint rridx = 0;
    uint lastRridx = 0;
    Box closestBox;

    closest.col = vec3(0.);

    while ((first || closest.exists) && (path < FOG_DIST)) 
    {
        float minDist = POS_INF;
        closest.exists = false;

        for (int i = 0; i < TRIVOX_MAX_ROOMS; ++i) 
        {
            RoomRef rr = roomrefs_data[i];

            if ((rr.idx == 0) || (!first && lastRridx == i))
                continue;

            Room r = rooms_data[rr.idx - 1];
            Box box = Box(rr.mat[3].xyz, mat3(rr.mat), r.sz);
            Intersection inter = raytrace_box(ray, box);

            if (inter.exists) {
                float dist = length(inter.o - ray.o);
                if (dist < minDist) {
                    minDist = dist;
                    closest = inter;
                    rridx = i;
                    closestBox = box;
                }
            }
        }

        if (closest.exists) {
            lastRridx = rridx;
            path += length(ray.o - closest.o);
            ray.o = closest.o;
            
            Intersection local = raytrace_room(ray, roomrefs_data[rridx], closestBox);

            if (local.exists) {
                path += length(ray.o - local.o);
                closest = local;
                hit = true;
                break;
            } else {
                path += length(ray.o - closest.exit.o);
            }

            
            ray = closest.exit;
        }

        first = false;
        //break;
    }

    if (!hit)
        path = FOG_DIST;

    closest.col = mix(.5 * (closest.n + 1.), FOG_COLOR, clamp(path / FOG_DIST, .001, 1.));

    return closest;
}

void main() 
{
    vec3 start = CAM_POS;
    vec2 coeffs = (gl_FragCoord.xy - RESOLUTION * .5) / (RESOLUTION.y * .5);
    vec4 dirw = CAM_MVP * vec4(coeffs.x, coeffs.y, 1.0, 1.0);
    vec3 dir = normalize(dirw.xyz/dirw.w - start);
    vec3 col = texture(texture0, fragTexCoord).rgb;

    Ray ray = Ray(start, dir);

    Intersection inter = raytrace_rooms(ray);

    vec3 frontCol = texture(texture1, fragTexCoord).rgb;
    if (length(frontCol) != 0)
        outColor = vec4(frontCol, 1.0);        
    else if (length(inter.col) == 0)
        outColor = vec4(col, 1.0);
    else
        outColor = vec4(inter.col, 1.0);
        
}