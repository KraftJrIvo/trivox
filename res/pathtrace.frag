#version 430

const float PI = 3.14159265358979323846;
const float EPS = 0.002;
const float LIGHT_INTENSITY = 6.0;
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
uniform vec2 RESOLUTION;
uniform mat4 CAM_MVP;
uniform vec3 CAM_POS;
uniform int N_SHAPES;
uniform int N_LIGHTS;
uniform int SAMPLE_INDEX;
uniform int SPP_PER_FRAME;
uniform int MAX_BOUNCES;

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

uint nextRandom(inout uint state) {
    state = state * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float randomFloat(inout uint state) {
    return float(nextRandom(state)) * (1.0 / 4294967296.0);
}

vec2 randomVec2(inout uint state) {
    return vec2(randomFloat(state), randomFloat(state));
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

bool intersectSphere(Ray ray, vec3 center, float radius, out float t, out vec3 normal) {
    vec3 oc = ray.origin - center;
    float h = dot(oc, ray.direction);
    float discriminant = h * h - (dot(oc, oc) - radius * radius);
    if (discriminant <= 0.0) return false;

    float root = sqrt(discriminant);
    t = -h - root;
    if (t <= EPS) t = -h + root;
    if (t <= EPS) return false;

    vec3 position = ray.origin + ray.direction * t;
    normal = normalize(position - center);
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

Hit traceScene(Ray ray, float maximumDistance) {
    Hit best;
    best.exists = false;
    best.t = maximumDistance;
    best.position = vec3(0.0);
    best.normal = vec3(0.0, 1.0, 0.0);
    best.color = vec3(0.0);
    best.material = MATERIAL_DIFFUSE;
    best.shapeIndex = -1;

    for (int i = 0; i < N_SHAPES; ++i) {
        Shape shape = shapes_data[i];
        float t = maximumDistance;
        vec3 normal = vec3(0.0);
        bool exists = false;

        if (shape.type == SHAPE_SPHERE) {
            exists = intersectSphere(ray, verts_data[shape.vid0], verts_data[shape.vid1].x, t, normal);
        } else if (shape.type == SHAPE_QUAD) {
            exists = intersectQuad(ray,
                                   verts_data[shape.vid0],
                                   verts_data[shape.vid1],
                                   verts_data[shape.vid2],
                                   t, normal);
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

bool selectLight(int lightIndex, out int shapeIndex, out vec3 center, out float radius, out vec3 emission) {
    int currentLight = 0;
    for (int i = 0; i < N_SHAPES; ++i) {
        Shape shape = shapes_data[i];
        if (shape.matIdx == MATERIAL_EMISSIVE && shape.type == SHAPE_SPHERE) {
            if (currentLight == lightIndex) {
                shapeIndex = i;
                center = verts_data[shape.vid0];
                radius = verts_data[shape.vid1].x;
                emission = shape.col * LIGHT_INTENSITY;
                return true;
            }
            ++currentLight;
        }
    }
    return false;
}

vec3 sampleDirectLight(Hit surface, inout uint randomState) {
    if (N_LIGHTS <= 0) return vec3(0.0);

    int selectedLight = min(int(randomFloat(randomState) * float(N_LIGHTS)), N_LIGHTS - 1);
    int lightShape = -1;
    vec3 lightCenter = vec3(0.0);
    float lightRadius = 0.0;
    vec3 emission = vec3(0.0);
    if (!selectLight(selectedLight, lightShape, lightCenter, lightRadius, emission)) return vec3(0.0);

    vec3 toLight = lightCenter - surface.position;
    float distanceSquared = dot(toLight, toLight);
    if (distanceSquared <= lightRadius * lightRadius + EPS) return vec3(0.0);

    float distanceToLight = sqrt(distanceSquared);
    vec3 coneAxis = toLight / distanceToLight;
    float cosThetaMax = sqrt(max(0.0, 1.0 - lightRadius * lightRadius / distanceSquared));
    vec2 sampleValue = randomVec2(randomState);
    float cosTheta = mix(1.0, cosThetaMax, sampleValue.x);
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    float phi = 2.0 * PI * sampleValue.y;
    vec3 tangent, bitangent;
    buildBasis(coneAxis, tangent, bitangent);
    vec3 lightDirection = normalize(tangent * (cos(phi) * sinTheta) +
                                    bitangent * (sin(phi) * sinTheta) +
                                    coneAxis * cosTheta);
    float surfaceCosine = max(dot(surface.normal, lightDirection), 0.0);
    if (surfaceCosine <= 0.0) return vec3(0.0);

    Ray shadowRay = Ray(surface.position + surface.normal * (4.0 * EPS), lightDirection);
    Hit blocker = traceScene(shadowRay, 1000.0);
    if (!blocker.exists || blocker.shapeIndex != lightShape) return vec3(0.0);

    float solidAngle = 2.0 * PI * (1.0 - cosThetaMax);
    float inversePdf = float(N_LIGHTS) * solidAngle;
    vec3 brdf = surface.color / PI;
    return emission * brdf * surfaceCosine * inversePdf;
}

vec3 tracePath(Ray ray, inout uint randomState) {
    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);
    bool previousWasSpecular = true;

    for (int bounce = 0; bounce < MAX_BOUNCES; ++bounce) {
        Hit hit = traceScene(ray, 1000.0);
        if (!hit.exists) break;

        if (hit.material == MATERIAL_EMISSIVE) {
            if (previousWasSpecular) radiance += throughput * hit.color * LIGHT_INTENSITY;
            break;
        }

        if (hit.material == MATERIAL_MIRROR) {
            throughput *= hit.color;
            vec3 reflected = normalize(reflect(ray.direction, hit.normal));
            ray = Ray(hit.position + hit.normal * (4.0 * EPS), reflected);
            previousWasSpecular = true;
        } else {
            radiance += throughput * sampleDirectLight(hit, randomState);
            throughput *= hit.color;

            vec3 tangent, bitangent;
            buildBasis(hit.normal, tangent, bitangent);
            vec3 localDirection = cosineHemisphere(randomVec2(randomState));
            vec3 nextDirection = normalize(tangent * localDirection.x +
                                           bitangent * localDirection.y +
                                           hit.normal * localDirection.z);
            ray = Ray(hit.position + hit.normal * (4.0 * EPS), nextDirection);
            previousWasSpecular = false;
        }

        if (bounce >= 3) {
            float survival = clamp(max(throughput.r, max(throughput.g, throughput.b)), 0.05, 0.95);
            if (randomFloat(randomState) > survival) break;
            throughput /= survival;
        }
    }

    return radiance;
}

Ray makeCameraRay(vec2 pixelPosition) {
    vec2 coefficients = (pixelPosition - RESOLUTION * 0.5) / (RESOLUTION.y * 0.5);
    vec4 worldPoint = CAM_MVP * vec4(coefficients.x, coefficients.y, 1.0, 1.0);
    vec3 direction = normalize(worldPoint.xyz / worldPoint.w - CAM_POS);
    return Ray(CAM_POS, direction);
}

void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec3 previousAverage = texelFetch(texture0, pixel, 0).rgb;
    vec3 frameSum = vec3(0.0);

    for (int sampleOffset = 0; sampleOffset < SPP_PER_FRAME; ++sampleOffset) {
        int absoluteSample = SAMPLE_INDEX + sampleOffset;
        uint randomState = uint(pixel.x + pixel.y * int(RESOLUTION.x));
        randomState ^= uint(absoluteSample + 1) * 277803737u;
        randomState ^= 0x9e3779b9u;

        vec2 jitter = randomVec2(randomState) - 0.5;
        Ray cameraRay = makeCameraRay(gl_FragCoord.xy + jitter);
        frameSum += tracePath(cameraRay, randomState);
    }

    float previousSamples = float(SAMPLE_INDEX);
    float addedSamples = float(SPP_PER_FRAME);
    vec3 average = (previousAverage * previousSamples + frameSum) /
                   max(previousSamples + addedSamples, 1.0);
    outColor = vec4(average, 1.0);
}
