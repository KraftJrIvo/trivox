#version 430

const float PI = 3.14159265358979323846;
const float EPS = 0.002;
const float LIGHT_INTENSITY = 6.0;
const int MIN_LVL = 0;
const int MAX_LVL = 3;
const uint SHAPE_SPHERE = 2u;
const uint SHAPE_QUAD = 6u;
const uint MATERIAL_DIFFUSE = 0u;
const uint MATERIAL_EMISSIVE = 1u;
const uint MATERIAL_MIRROR = 2u;

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 outColor;

struct Shape {
    uint type;
    uint vid0;
    uint vid1;
    uint vid2;
    vec3 col;
    uint matIdx;
};

layout(std140, binding = 0) buffer Vertices { vec3 verts_data[]; };
layout(std140, binding = 1) buffer Shapes { Shape shapes_data[]; };

uniform sampler2D texture0;
uniform int N_PROBE_EXTRA_LVLS;
uniform int ITERATION;
uniform int LVL_0_RES;
uniform int N_SHAPES;

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Hit {
    bool exists;
    float t;
    vec3 position;
    vec3 normal;
    vec3 color;
    uint material;
    int shapeIndex;
};

vec2 lambertAzimuthalForward(vec3 direction) {
    if (direction.y > 0.999999) return vec2(0.0, 1.0);
    float denominator = max(1e-6, 1.0 - direction.y);
    return 0.5 * sqrt(2.0 / denominator) * direction.xz;
}

vec3 lambertAzimuthalInverse(vec2 disk) {
    float radius = length(disk);
    if (radius < 1e-6) return vec3(0.0, -1.0, 0.0);
    float angle = 2.0 * asin(clamp(radius, 0.0, 1.0));
    float sine = sin(angle);
    return vec3((disk.x / radius) * sine,
                -cos(angle),
                (disk.y / radius) * sine);
}

vec4 sampleProbe(int level, vec3 cell, vec3 direction) {
    int levelSide = LVL_0_RES * (1 << N_PROBE_EXTRA_LVLS) * 8;
    int cellSize = (1 << level) * levelSide /
                   (8 * (1 << N_PROBE_EXTRA_LVLS));
    vec2 cellOrigin = vec2(level * levelSide + cellSize * cell.x,
                           cell.z * levelSide + cell.y * cellSize);

    vec2 disk = lambertAzimuthalForward(direction);
    float safeRadius = max(0.0, 1.0 - 1.5 / float(cellSize));
    float diskRadius = length(disk);
    if (diskRadius > safeRadius)
        disk *= safeRadius / max(diskRadius, 1e-6);

    vec2 localPixel = (disk + 1.0) * 0.5 * float(cellSize);
    localPixel = clamp(localPixel, vec2(0.5), vec2(float(cellSize) - 0.5));
    return texture(texture0, (cellOrigin + localPixel) / vec2(textureSize(texture0, 0)));
}

float probeSideWeight(vec3 probePosition, vec3 surfacePosition,
                      vec3 surfaceNormal, float cellSize) {
    float side = dot(probePosition - surfacePosition, surfaceNormal);
    return smoothstep(-0.15 * cellSize, 0.35 * cellSize, side);
}

vec4 interpolateProbeLevel(int level, vec3 position, vec3 normal, vec3 direction) {
    int probeCount = 1 << ((MAX_LVL - level) + N_PROBE_EXTRA_LVLS);
    float cellSize = 8.0 / float(probeCount);
    vec3 probeCoordinate = position / cellSize - 0.5;
    vec3 baseProbe = floor(probeCoordinate);
    vec3 fraction = fract(probeCoordinate);
    vec4 weightedValue = vec4(0.0);
    float weightSum = 0.0;

    for (int z = 0; z < 2; ++z) {
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                vec3 corner = vec3(x, y, z);
                vec3 probe = clamp(baseProbe + corner,
                                   vec3(0.0), vec3(float(probeCount - 1)));
                vec3 axisWeight = mix(vec3(1.0) - fraction, fraction, corner);
                float weight = axisWeight.x * axisWeight.y * axisWeight.z;
                vec3 probePosition = (probe + 0.5) * cellSize;
                weight *= probeSideWeight(probePosition, position, normal, cellSize);
                weightedValue += sampleProbe(level, probe, direction) * weight;
                weightSum += weight;
            }
        }
    }

    if (weightSum < 1e-5) {
        vec3 nearest = clamp(round(probeCoordinate),
                             vec3(0.0), vec3(float(probeCount - 1)));
        return sampleProbe(level, nearest, direction);
    }
    return weightedValue / weightSum;
}

vec3 samplePreviousIndirectRay(vec3 position, vec3 normal, vec3 direction) {
    vec3 radiance = vec3(0.0);
    float opacity = 0.0;
    for (int level = MIN_LVL; level <= MAX_LVL; ++level) {
        vec4 segment = interpolateProbeLevel(level, position, normal, direction);
        radiance += (1.0 - opacity) * segment.rgb;
        opacity += (1.0 - opacity) * segment.a;
        if (opacity >= 0.999) break;
    }
    return radiance;
}

void buildBasis(vec3 normal, out vec3 tangent, out vec3 bitangent) {
    vec3 up = (abs(normal.z) < 0.999) ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
    tangent = normalize(cross(up, normal));
    bitangent = cross(normal, tangent);
}

vec3 cosineHemisphere(vec2 u) {
    float radius = sqrt(u.x);
    float angle = 2.0 * PI * u.y;
    return vec3(radius * cos(angle), radius * sin(angle), sqrt(max(0.0, 1.0 - u.x)));
}

vec3 samplePreviousIndirect(Hit surface) {
    const int SAMPLE_COUNT = 4;
    const float GOLDEN_RATIO_CONJUGATE = 0.61803398875;
    vec3 tangent, bitangent;
    buildBasis(surface.normal, tangent, bitangent);
    vec3 accumulated = vec3(0.0);
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        vec2 xi = vec2((float(i) + 0.5) / float(SAMPLE_COUNT),
                       fract((float(i) + 0.5) * GOLDEN_RATIO_CONJUGATE));
        vec3 localDirection = cosineHemisphere(xi);
        vec3 direction = normalize(tangent * localDirection.x +
                                   bitangent * localDirection.y +
                                   surface.normal * localDirection.z);
        accumulated += samplePreviousIndirectRay(surface.position + surface.normal * (4.0 * EPS),
                                                  surface.normal, direction);
    }
    return accumulated / float(SAMPLE_COUNT);
}

bool intersectSphere(Ray ray, vec3 center, float radius, out float t, out vec3 normal) {
    vec3 oc = ray.origin - center;
    float h = dot(oc, ray.direction);
    float discriminant = h * h - (dot(oc, oc) - radius * radius);
    if (discriminant <= 0.0) return false;
    float root = sqrt(discriminant);
    t = -h - root;
    if (t <= EPS) t = -h + root;
    if (t <= EPS) return false;
    normal = normalize(ray.origin + ray.direction * t - center);
    if (dot(normal, ray.direction) > 0.0) normal = -normal;
    return true;
}

bool intersectQuad(Ray ray, vec3 v0, vec3 v1, vec3 v2, out float t, out vec3 normal) {
    vec3 v3 = v0 + v2 - v1;
    vec3 geometricNormal = normalize(cross(v1 - v0, v2 - v1));
    float denominator = dot(ray.direction, geometricNormal);
    if (abs(denominator) < EPS) return false;
    t = dot(v0 - ray.origin, geometricNormal) / denominator;
    if (t <= EPS) return false;

    vec3 position = ray.origin + ray.direction * t;
    vec3 edges[4] = vec3[4](v1 - v0, v2 - v1, v3 - v2, v0 - v3);
    vec3 vertices[4] = vec3[4](v0, v1, v2, v3);
    bool hasPositive = false;
    bool hasNegative = false;
    for (int i = 0; i < 4; ++i) {
        float side = dot(cross(edges[i], position - vertices[i]), geometricNormal);
        hasPositive = hasPositive || side > EPS;
        hasNegative = hasNegative || side < -EPS;
    }
    if (hasPositive && hasNegative) return false;
    normal = (denominator < 0.0) ? geometricNormal : -geometricNormal;
    return true;
}

Hit traceScene(Ray ray, float maximumDistance, int ignoredShape) {
    Hit best;
    best.exists = false;
    best.t = maximumDistance;
    best.position = vec3(0.0);
    best.normal = vec3(0.0, 1.0, 0.0);
    best.color = vec3(0.0);
    best.material = MATERIAL_DIFFUSE;
    best.shapeIndex = -1;

    for (int i = 0; i < N_SHAPES; ++i) {
        if (i == ignoredShape) continue;
        Shape shape = shapes_data[i];
        float t = maximumDistance;
        vec3 normal = vec3(0.0);
        bool exists = false;
        if (shape.type == SHAPE_SPHERE) {
            exists = intersectSphere(ray, verts_data[shape.vid0], verts_data[shape.vid1].x, t, normal);
        } else if (shape.type == SHAPE_QUAD) {
            exists = intersectQuad(ray, verts_data[shape.vid0], verts_data[shape.vid1],
                                   verts_data[shape.vid2], t, normal);
        }
        if (exists && t < best.t && t < maximumDistance) {
            best.exists = true;
            best.t = t;
            best.position = ray.origin + ray.direction * t;
            best.normal = normal;
            best.color = shape.col;
            best.material = shape.matIdx;
            best.shapeIndex = i;
        }
    }
    return best;
}

vec3 directIrradiance(Hit surface) {
    vec3 irradiance = vec3(0.0);
    for (int i = 0; i < N_SHAPES; ++i) {
        Shape light = shapes_data[i];
        if (light.matIdx != MATERIAL_EMISSIVE || light.type != SHAPE_SPHERE) continue;

        vec3 center = verts_data[light.vid0];
        float radius = verts_data[light.vid1].x;
        vec3 toCenter = center - surface.position;
        float distanceSquared = dot(toCenter, toCenter);
        if (distanceSquared <= radius * radius + EPS) continue;
        float distanceToCenter = sqrt(distanceSquared);
        vec3 direction = toCenter / distanceToCenter;
        float cosine = max(dot(surface.normal, direction), 0.0);
        if (cosine <= 0.0) continue;

        Ray shadowRay = Ray(surface.position + surface.normal * (4.0 * EPS), direction);
        Hit blocker = traceScene(shadowRay, distanceToCenter - radius - 4.0 * EPS, i);
        if (blocker.exists) continue;

        float sinSquared = clamp(radius * radius / distanceSquared, 0.0, 0.9999);
        float solidAngle = 2.0 * PI * (1.0 - sqrt(1.0 - sinSquared));
        irradiance += light.col * LIGHT_INTENSITY * cosine * (solidAngle / PI);
    }
    return irradiance;
}

void main() {
    int levelSide = LVL_0_RES * (1 << N_PROBE_EXTRA_LVLS) * 8;
    int level = int(floor(gl_FragCoord.x / float(levelSide)));
    int cellSize = (1 << level) * levelSide / (8 * (1 << N_PROBE_EXTRA_LVLS));
    float xWithinLevel = gl_FragCoord.x - float(level * levelSide);
    float z = floor(gl_FragCoord.y / float(levelSide));
    float yWithinSlice = gl_FragCoord.y - z * float(levelSide);
    float x = floor(xWithinLevel / float(cellSize));
    float y = floor(yWithinSlice / float(cellSize));
    vec3 cell = vec3(x, y, z);
    vec2 disk = 2.0 * mod(vec2(xWithinLevel, yWithinSlice), float(cellSize)) /
                float(cellSize) - 1.0;

    int probeCount = 1 << ((MAX_LVL - level) + N_PROBE_EXTRA_LVLS);
    int activeZSlices = probeCount;
    if (dot(disk, disk) > 1.0 || z >= float(activeZSlices)) {
        outColor = vec4(0.0);
        return;
    }

    float probeSpacing = 8.0 / float(probeCount);
    vec3 probePosition = probeSpacing * (cell + 0.5);
    vec3 direction = lambertAzimuthalInverse(disk);
    float intervalStart = (level == 0) ? 0.0 : exp2(float(level - 1));
    float intervalEnd = (level == MAX_LVL) ? 16.0 : exp2(float(level));
    Ray ray = Ray(probePosition + direction * intervalStart, direction);
    Hit hit = traceScene(ray, intervalEnd - intervalStart, -1);

    if (!hit.exists) {
        outColor = vec4(0.0);
        return;
    }

    vec3 outgoing = vec3(0.0);
    if (hit.material == MATERIAL_DIFFUSE) {
        if (ITERATION == 0) {
            outgoing = hit.color * directIrradiance(hit);
        } else {
            vec3 directOutgoing = texelFetch(texture0, ivec2(gl_FragCoord.xy), 0).rgb;
            outgoing = directOutgoing + hit.color * samplePreviousIndirect(hit);
        }
    }

    outColor = vec4(outgoing, 1.0);
}
