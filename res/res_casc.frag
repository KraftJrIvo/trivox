#version 430

const float EPS = 1e-3;
const uint TRIVOX_MAX_ROOMS = 16u;
const float POS_INF = 1.0 / 0.0;
const float NEG_INF = -1.0 / 0.0;
const float MIN_VSZ = 100.0;
const int MIN_LVL = 0;
const int MAX_LVL = 3;
const float FOG_DIST = 100.0;
const vec3 FOG_COLOR = vec3(0.0);

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 outColor;

struct Room { vec3 sz; uint firstCellIdx; };
struct RoomRef { mat4 mat; vec3 col; uint idx; };
struct Shape { uint type; uint vid0; uint vid1; uint vid2; vec3 col; uint matIdx; };
struct Cell { uvec4 shids[64]; uint dist; uint nShapes; };

layout(std140, binding = 0) buffer Vertices { vec3 verts_data[]; };
layout(std140, binding = 1) buffer Shapes   { Shape shapes_data[]; };
layout(std140, binding = 2) buffer Rooms    { Room rooms_data[]; };
layout(std140, binding = 3) buffer RoomRefs { RoomRef roomrefs_data[]; };
layout(std140, binding = 4) buffer Cells    { Cell cells_data[]; };

uniform vec2 RESOLUTION;

uniform sampler2D texture0;

uniform int N_PROBE_EXTRA_LVLS;
uniform int N_ITERS;
uniform int LVL_0_RES;

struct Sphere { vec3 o; float r; vec3 col; };
struct Box    { vec3 o; mat3 rot; vec3 sz; };
struct AAC    { vec3 o; float side; };
struct Ray    { vec3 o; vec3 dir; };
struct Intersection {
    bool exists;
    vec3 o;
    vec3 n;
    vec3 col;
    Ray exit;
};

Cell cell_at(uint room, uint lvl, vec3 pos) {
    uint ncells = 1u << lvl;
    float cellsz = exp2(float(MAX_LVL)) / float(ncells);
    vec3 ipos = floor(pos / cellsz);
    uint roomsz = (1u << (MIN_LVL * 3)) * ((1u << ((MAX_LVL - MIN_LVL + 1) * 3)) - 1u) / 7u;
    uint lvloff = uint(pow(8.0, float(MIN_LVL)) * (pow(8.0, float((int(lvl) - 1) - MIN_LVL + 1)) - 1.0) / 7.0);
    uint celloff = uint(ncells * ncells * uint(ipos.z) + ncells * uint(ipos.y) + uint(ipos.x));
    return cells_data[roomsz * room + lvloff + celloff];
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
    vec2 pix = celpos + lambertAzimuthalForward(dir) * cellsz;
    return texture(texture0, pix / textureSize(texture0, 0).xy);
}

vec3 getLightFrom(uint room, uint lvl, vec3 pos, vec3 dir) {
    vec3 col = vec3(0.);
    float alpha = 0.;
    for (int i = MIN_LVL; i <= lvl; ++i) {
        int n3dcells = (1 << ((MAX_LVL - i) + N_PROBE_EXTRA_LVLS));
        float cell3dsz = 8. / float(n3dcells);
        vec3 pf000 = clamp(floor(pos / cell3dsz - 0.5), vec3(0.), vec3(float(n3dcells)));
        vec3 pf111 = clamp(pf000 + vec3(1.), vec3(0.), vec3(float(n3dcells)));
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
        vec3 uvw = (pos - pf000 * cell3dsz) / cell3dsz;
        vec4 val = trilinear(f000, f100, f010, f110, f001, f101, f011, f111, uvw);
        col = mix(col, val.rgb, val.a);
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
    //vec3 lightdir = normalize(vec3(4.0) - res.o);
    //vec3 amb = 0.1 * sph.col;
    //vec3 dif = sph.col * max(dot(lightdir, res.n), 0.0);
    //vec3 spc = vec3(pow(max(0.0, dot(reflect(-lightdir, res.n), -ray.dir)), 50.0)) * 0.33;
    //res.col = amb + dif + spc;
    vec3 amb = vec3(0.);
    vec3 lightdir = reflect(ray.dir, res.n);
    vec3 dif = sph.col * getLightFrom(0, MAX_LVL, res.o, lightdir) * max(dot(lightdir, res.n), 0.0);
    res.col = light ? sph.col : vec3(0);
    return res;
}

Intersection raytrace_box(Ray ray, Box box) {
    Intersection res; res.exists = false;
    vec3 lro = (ray.o - box.o) * box.rot;
    vec3 lrd = ray.dir * box.rot;
    vec3 invd = 1.0 / lrd;
    vec3 t1 = (-lro) * invd;
    vec3 t2 = (box.sz - lro) * invd;
    vec3 ta = min(t1, t2);
    vec3 tb = max(t1, t2);
    float tNear = max(max(ta.x, ta.y), ta.z);
    float tFar  = min(min(tb.x, tb.y), tb.z);
    bool inside = all(greaterThanEqual(lro, vec3(0.0))) && all(lessThanEqual(lro, box.sz));
    if (!(inside || (tNear <= tFar && tNear >= 0.0))) return res;

    res.exists = true;
    vec3 localPos = lro + lrd * (inside ? 0.0 : tNear);
    res.o = box.rot * localPos + box.o;
    vec3 locn = vec3(0.0);
    float eps = 0.001;
    if (abs(localPos.x) < eps) locn.x = -1.0;
    else if (abs(localPos.x - box.sz.x) < eps) locn.x = 1.0;
    else if (abs(localPos.y) < eps) locn.y = -1.0;
    else if (abs(localPos.y - box.sz.y) < eps) locn.y = 1.0;
    else if (abs(localPos.z) < eps) locn.z = -1.0;
    else if (abs(localPos.z - box.sz.z) < eps) locn.z = 1.0;
    res.n = box.rot * locn;
    vec3 localExit = lro + lrd * tFar;
    res.exit = Ray(box.rot * localExit + box.o, ray.dir);
    if (inside) res.o = ray.o;
    return res;
}

Intersection raytrace_aac(Ray ray, AAC aac) {
    Intersection res; res.exists = false;
    vec3 mn = aac.o;
    vec3 mx = aac.o + vec3(aac.side);
    vec3 invd = 1.0 / ray.dir;
    vec3 t1 = (mn - ray.o) * invd;
    vec3 t2 = (mx - ray.o) * invd;
    vec3 ta = min(t1, t2);
    vec3 tb = max(t1, t2);
    float t_near = max(max(ta.x, ta.y), ta.z);
    float t_far  = min(min(tb.x, tb.y), tb.z);
    bool inside = (ray.o.x >= mn.x && ray.o.x <= mx.x &&
                   ray.o.y >= mn.y && ray.o.y <= mx.y &&
                   ray.o.z >= mn.z && ray.o.z <= mx.z);
    if (!(inside || (t_near <= t_far && t_near > 0.0))) return res;
    res.exists = true;
    res.o = ray.o + (inside ? 0.0 : t_near) * ray.dir;
    vec3 pos = res.o - aac.o;
    vec3 n = vec3(0.0);
    float eps = 0.001;
    if (abs(pos.x) < eps) n = vec3(-1.0, 0.0, 0.0);
    else if (abs(pos.x - aac.side) < eps) n = vec3(1.0, 0.0, 0.0);
    else if (abs(pos.y) < eps) n = vec3(0.0, -1.0, 0.0);
    else if (abs(pos.y - aac.side) < eps) n = vec3(0.0, 1.0, 0.0);
    else if (abs(pos.z) < eps) n = vec3(0.0, 0.0, -1.0);
    else if (abs(pos.z - aac.side) < eps) n = vec3(0.0, 0.0, 1.0);
    res.n = n;
    res.col = vec3(1.0);
    res.exit = Ray(ray.o + t_far * ray.dir, ray.dir);
    return res;
}

float minval3(vec3 v) { return min(v.x, min(v.y, v.z)); }

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

void main() 
{
    int lvlside = LVL_0_RES * (1 << N_PROBE_EXTRA_LVLS) * 8;
    int roomside = (lvlside * (MAX_LVL - MIN_LVL + 1 + N_PROBE_EXTRA_LVLS));
    int room = int(floor(gl_FragCoord.x / roomside));
    int lvl = int(floor((gl_FragCoord.x - room * roomside) / lvlside));
    int cellsz = (1 << lvl) * lvlside / (8 * (1 << N_PROBE_EXTRA_LVLS));
    float xx = gl_FragCoord.x - room * roomside - lvl * lvlside;
    float z = floor(gl_FragCoord.y / lvlside);
    float yy = gl_FragCoord.y - z * lvlside;
    float x = int(floor(xx / cellsz));
    float y = int(floor(yy / cellsz));
    vec3 cell = vec3(x, y, z);
    vec2 pcrd = 2. * mod(vec2(xx, yy), float(cellsz)) / float(cellsz) - 1.;
    int n3dcells = (1 << ((MAX_LVL - lvl) + N_PROBE_EXTRA_LVLS));
    float cell3dsz = 8. / float(n3dcells);
    float prvlen = float((lvl + N_PROBE_EXTRA_LVLS == 0) ? 0 : (1 << (lvl + N_PROBE_EXTRA_LVLS - 1)));
    float len = 10.0;//float((1 << (lvl + N_PROBE_EXTRA_LVLS)));
    vec3 center = cell3dsz * (cell + .5);
    vec3 dir = lambertAzimuthalInverse(pcrd);
    vec3 start = center + dir * prvlen;
    Ray ray = Ray(/*start*/center, dir);
    Intersection inter = raytrace_room(uint(room), ray, center, len);
    if (/*length(pcrd) < 1. && */z < (1 << ((MAX_LVL - lvl))))
        outColor = vec4(inter.col, inter.exists ? 1.0 : 0.0);
}
