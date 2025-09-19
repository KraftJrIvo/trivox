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

struct Shape {
    uint type;
    uint vid0;
    uint vid1;
    uint vid2;
    vec3 col;
    uint matIdx;
};

struct Cell {
    uvec4 shids[64];
    uint dist;
    uint nShapes;
};


layout(std140, binding = 0) 
buffer Vertices
{
    vec3 verts_data[];
};
layout(std140, binding = 1) 
buffer Shapes
{
    Shape shapes_data[];
};
layout(std140, binding = 2) 
buffer Rooms
{
    Room rooms_data[];
};
layout (std140, binding = 3) 
buffer RoomRefs {
    RoomRef roomrefs_data[];
};
layout (std140, binding = 4) 
buffer Cells {
    Cell cells_data[];
};


uniform vec2 RESOLUTION;
uniform float TIME;

uniform mat4 CAM_MVP;
uniform float CAM_FOV;
uniform vec3 CAM_POS;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;

uniform int N_PROBE_EXTRA_LVLS;
uniform int N_ITERS;
uniform int LVL_0_RES;

struct Sphere {
    vec3 o;
    float r;
    vec3 col;
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



Cell cell_at(uint room, uint lvl, vec3 pos) {
    float ncells = 1 << lvl;
    float cellsz = pow(2,float(MAX_LVL)) / ncells;
    vec3 ipos = floor(pos / cellsz);
    uint roomsz = (1 << (MIN_LVL * 3)) * ((1 << ((MAX_LVL - MIN_LVL + 1) * 3)) - 1) / (8 - 1);
    uint roomoff = roomsz * room;
    uint lvloff = uint(pow(8, MIN_LVL) * (pow(8, (lvl - 1) - MIN_LVL + 1) - 1) / (8 - 1));
    uint celloff = uint(ncells * ncells * ipos.z + ncells * ipos.y + ipos.x);
    return cells_data[roomoff + lvloff + celloff];
}

vec2 lambertAzimuthalForward(vec3 p) {
    float denom = 1.0 - p.y;
    float factor = sqrt(2.0 / denom);
    return 0.5 * factor * p.xz;
}

vec3 lambertAzimuthalInverse(vec2 p) {
    float rho = length(p);
    if (rho < 1e-6) {
        return vec3(0.0, -1.0, 0.0);
    }
    float rho2 = rho * 2.0;
    float c = 2.0 * asin(rho2 * 0.5);
    float sc = sin(c);
    return vec3(
        (p.x / rho) * sc,
        -cos(c),
        (p.y / rho) * sc
    );
}

vec4 trilinear(
    vec4 f000, vec4 f100,
    vec4 f010, vec4 f110,
    vec4 f001, vec4 f101,
    vec4 f011, vec4 f111,
    vec3 uvw
) {
    float u = uvw.x;
    float v = uvw.y;
    float w = uvw.z;

    vec4 c00 = mix(f000, f100, u);
    vec4 c10 = mix(f010, f110, u);
    vec4 c01 = mix(f001, f101, u);
    vec4 c11 = mix(f011, f111, u);

    vec4 c0 = mix(c00, c10, v);
    vec4 c1 = mix(c01, c11, v);

    return mix(c0, c1, w);
}

vec4 getProbeVal(uint room, uint lvl, vec3 cell, vec3 dir) {
    int lvlside = LVL_0_RES * (1 << N_PROBE_EXTRA_LVLS) * 8;
    int roomside = (lvlside * (MAX_LVL - MIN_LVL + 1 + N_PROBE_EXTRA_LVLS));
    int cellsz = (1 << lvl) * lvlside / 8;
    vec2 celpos = vec2(room * roomside + lvl * lvlside + cellsz * cell.x, cell.z * lvlside + cell.y * cellsz);
    vec2 pix = celpos + (lambertAzimuthalForward(dir) + 1.) * .5 * cellsz;
    return texture(texture2, pix / textureSize(texture2, 0).xy);
}

vec3 getLightFrom(uint room, uint lvl, vec3 pos, vec3 dir) {
    vec3 col = vec3(0.);
    float alpha = 0.;
    for (int i = MIN_LVL; i <= lvl; ++i) {
        int n3dcells = (1 << ((MAX_LVL - i) + N_PROBE_EXTRA_LVLS));
        float cell3dsz = 8. / float(n3dcells);
        vec3 pf000 = clamp(floor(pos / cell3dsz - 0.5), vec3(0.), vec3(float(n3dcells - 1)));
        vec3 pf111 = clamp(pf000 + vec3(1.), vec3(0.), vec3(float(n3dcells - 1)));
        vec3 diff = pf111 - pf000;
        vec3 pf100 = pf000 + vec3(diff.x, 0., 0.);
        vec3 pf010 = pf000 + vec3(0., diff.y, 0.);
        vec3 pf001 = pf000 + vec3(0., 0., diff.z);
        vec3 pf110 = pf000 + vec3(diff.x, diff.y, 0.);
        vec3 pf011 = pf000 + vec3(0., diff.y, diff.z);
        vec3 pf101 = pf000 + vec3(diff.x, 0., diff.z);
        vec4 f000 = getProbeVal(room, i, pf000, dir);
        vec4 f100 = getProbeVal(room, i, pf100, dir);
        vec4 f010 = getProbeVal(room, i, pf010, dir);
        vec4 f001 = getProbeVal(room, i, pf001, dir);
        vec4 f110 = getProbeVal(room, i, pf110, dir);
        vec4 f011 = getProbeVal(room, i, pf011, dir);
        vec4 f101 = getProbeVal(room, i, pf101, dir);
        vec4 f111 = getProbeVal(room, i, pf111, dir);
        vec3 uvw = (((pos / cell3dsz - 0.5) - pf000) * cell3dsz) / cell3dsz;
        vec4 val = trilinear(f000, f100, f010, f110, f001, f101, f011, f111, uvw);
        //vec4 val = f000;
        //vec4 val = vec4(pf000 / 8., 1.);
        col = mix(col, val.rgb, val.a);
        //break;
        alpha += val.a;
        if (alpha >= 1.)
            break;
    }
    return col;
}

Intersection raytrace_sphere(Ray ray, Sphere sph, bool light) {
    Intersection res; res.exists = false;
    vec3 oc = ray.o - sph.o;
    float r = sph.r;
    float h = dot(oc, ray.dir); 
    float c = dot(oc, oc) - r * r;
    float disc = h * h - c;
    if (disc <= 0.0) return res;
    float t = -h - sqrt(disc);
    if (t <= 0.0) return res;
    res.exists = true;
    res.o = ray.o + t * ray.dir;
    res.n = normalize(res.o - sph.o);
    
    vec3 lightdir = normalize(vec3(4.0) - res.o);
    vec3 amb = 0.1 * sph.col;
    vec3 dif = sph.col * max(dot(lightdir, res.n), 0.0);
    vec3 spc = vec3(pow(max(0.0, dot(reflect(-lightdir, res.n), -ray.dir)), 50.0)) * 0.33;

    //vec3 amb = 0.1 * sph.col;
    //vec3 lightdir = reflect(ray.dir, res.n);
    //vec3 dif = sph.col * getLightFrom(0, MAX_LVL, res.o, lightdir) * max(dot(lightdir, res.n), 0.0);
    //res.col = light ? vec3(1.) : (amb + dif);
    
    //res.col = amb + dif + spc;
    //res.col = light ? vec3(1.) : getLightFrom(0, MAX_LVL, res.o, res.n);
    //res.col = light ? getProbeVal(0, 3, vec3(0), res.n).rgb : (amb + dif + spc);
    res.col = light ? sph.col : getLightFrom(0, MAX_LVL, res.o, res.n);
    
    return res;
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

float minval3(vec3 v) {
    return min(v.x, min(v.y, v.z));
}

Intersection raytrace_room(uint rid, Ray lray, vec3 lcampos, float max_dist) {
    Intersection res;
    res.exists = false;

    Room r = rooms_data[rid];
    bool inside = all(greaterThanEqual(lray.o, vec3(0.))) && all(lessThanEqual(lray.o, r.sz));

    bool first = true;
    vec3 start = lray.o;

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
        float ncells = pow(2, MAX_LVL) / csz;

        Cell cell = cell_at(rid, uint(MAX_LVL - lvl), lray.o);

        //AAC aac = AAC(floor(lray.o / csz) * csz, csz);

        vec3 curcell = floor(lray.o / csz);
        int celrad = max(int(cell.dist) - 1, 0);
        vec3 startC = clamp(vec3(curcell - celrad), vec3(0.), vec3(ncells - 1.));
        vec3 endC = clamp(vec3(curcell + celrad), vec3(0.), vec3(ncells - 1.));
        AAC aac = AAC(startC * csz, minval3(endC - startC + 1) * csz);

        res = raytrace_aac(lray, aac);
        res.exists = false;

        Intersection bestres = res;
        bestres.exists = false;
        float besdist = length(res.exit.o - lray.o) + EPS;

        if (cell.dist == 0) {
            for (int i = 0; i < cell.nShapes; ++i) {
                Shape shape = shapes_data[cell.shids[i / 4][i % 4]];
                vec3 center = verts_data[shape.vid0];
                vec3 params = verts_data[shape.vid1];
                Sphere sph = Sphere(center, params[0], shape.col);
                res = raytrace_sphere(lray, sph, shape.matIdx == 1);
                float dist = length(res.o - lray.o);
                if (res.exists && dist < besdist) {
                    bestres = res;
                    besdist = dist;
                }
            }
            res = bestres;
        }


        if (res.exists) {            
            if (length(res.o - start) > max_dist)
                res.exists = false;
            return res;
        } else {         
            if (length(res.exit.o - start) > max_dist) {
                res.exists = false;   
                break;
            }
            lray.o = res.exit.o + res.exit.dir * EPS;
            inside = all(greaterThanEqual(lray.o, vec3(0.))) && all(lessThan(lray.o, r.sz));
            res.exists = false;
        }
    }

    return res;
}

Intersection raytrace_rooms(Ray ray, float max_dist) 
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
            if (path > max_dist) break;
            ray.o = closest.o;
            
            uint rid = roomrefs_data[rridx].idx - 1;
            Ray lray = Ray(clamp((ray.o - closestBox.o) * closestBox.rot, vec3(EPS), rooms_data[rid].sz - EPS), ray.dir * closestBox.rot);
            vec3 lcampos = (CAM_POS - closestBox.o) * closestBox.rot;
            Intersection local = raytrace_room(rid, lray, lcampos, max_dist - path);

            if (local.exists) {
                local.o = closestBox.rot * local.o + closestBox.o;
                path += length(ray.o - local.o);
                closest = local;
                if (path < max_dist) hit = true;
                break;
            } else {
                path += length(ray.o - closest.exit.o);
                if (path > max_dist) break;
            }
            
            ray = closest.exit;
        }

        first = false;
    }

    if (!hit)
        path = FOG_DIST;

    closest.col = mix(closest.col + EPS, FOG_COLOR, clamp(path / FOG_DIST, .001, 1.));

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

    Intersection inter = raytrace_rooms(ray, FOG_DIST);

    vec3 frontCol = texture(texture1, fragTexCoord).rgb;
    if (length(frontCol) != 0)
        outColor = vec4(frontCol, 1.0);        
    else if (length(inter.col) == 0)
        outColor = vec4(col, 1.0);
    else
        outColor = vec4(inter.col, 1.0);
        
}
