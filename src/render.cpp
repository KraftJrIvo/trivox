#include "render.h"
#include "raylib.h"
#include "raymath.h"
#include "rcamera.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#define SPEED 0.1f
#define FAST_COEFF 5.0f
#define RESIZE_CD 0.25f

extern "C" const unsigned char res_icon[];
extern "C" const size_t res_icon_len;

extern "C" const unsigned char res_raytrace_frag[];
extern "C" const unsigned char res_res_casc_frag[];
extern "C" const unsigned char res_pathtrace_frag[];
extern "C" const unsigned char res_pathtrace_display_frag[];

enum class RenderMode {
    Cascades,
    PathTrace
};

class RendererImpl : public Renderer {
    World::Ptr _w;
    Camera _cam;
    uvec2 _initSz;
    Vector2 _winSz, _baseWinSz, _sclWinSz;
    float _time, _lastReszTime;
    float _scale = 0.5f;
    Shader _raytraceShader, _resCascShader;
    Shader _pathtraceShader, _pathtraceDisplayShader;
    RenderTexture2D _backTex = {};
    RenderTexture2D _frontTex = {};
    RenderTexture2D _traceTex = {};
    RenderTexture2D _resCascTex[2] = {};
    RenderTexture2D _pathtraceAccumTex[2] = {};
    int _activeResCascTex = 0;
    int _activePathtraceTex = 0;
    int _pathtraceSampleCount = 0;
    int _pathtraceMaxBounces = 6;
    int _pathtraceSamplesPerFrame = 1;
    bool _pathtraceResetPending = true;
    bool _pathtraceAvailable = true;
    RenderMode _renderMode = RenderMode::Cascades;
    bool _drawGrids = false;
    bool _drawCasc = false;

    int res_casc_n_probe_extra_lvls = 0;
    int res_casc_lvl_0_res = 16;
    int res_casc_n_iterations = 2;

    Vector2 getResCascTexSz();

    void _resetCamPos();
    void _resetPathtraceAccumulation();
    void _unloadRenderTargets();
    void _updateShaderSize();
    void _input();
    void _drawRoomGrids(Vector3 campos, bool front = false);

  public:
    RendererImpl(World::Ptr w, uvec2 sz);
    ~RendererImpl() override;
    virtual void startRender() override;
};

static std::string getEnvironmentValue(const char *name) {
#if defined(_WIN32)
    char *value = nullptr;
    size_t valueLength = 0;
    _dupenv_s(&value, &valueLength, name);
    std::string result = value ? value : "";
    std::free(value);
    return result;
#else
    const char *value = std::getenv(name);
    return value ? value : "";
#endif
}

static int getEnvironmentInt(const char *name, int fallback, int minimum, int maximum) {
    const std::string value = getEnvironmentValue(name);
    if (value.empty()) return fallback;
    return std::clamp(std::atoi(value.c_str()), minimum, maximum);
}

static void requireCustomShader(Shader shader, const char *name) {
    if (!IsShaderValid(shader) || shader.id == rlGetShaderIdDefault())
        TraceLog(LOG_FATAL, "SHADER: Required %s shader failed to load", name);
}

static RenderTexture2D loadColorRenderTexture(int width, int height, int format) {
    RenderTexture2D target = {};
    target.id = rlLoadFramebuffer();
    if (target.id == 0) return target;

    rlEnableFramebuffer(target.id);
    target.texture.id = rlLoadTexture(nullptr, width, height, format, 1);
    target.texture.width = width;
    target.texture.height = height;
    target.texture.format = format;
    target.texture.mipmaps = 1;
    rlFramebufferAttach(target.id, target.texture.id,
                        RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    const bool complete = rlFramebufferComplete(target.id);
    rlDisableFramebuffer();
    if (!complete) {
        TraceLog(LOG_WARNING, "FBO: Color framebuffer format %i is incomplete", format);
        UnloadRenderTexture(target);
        return {};
    }
    return target;
}

static bool exportRenderTexture(RenderTexture2D target, const char *path) {
    if (target.id == 0) return false;
    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    const bool exported = ExportImage(image, path);
    UnloadImage(image);
    return exported;
}

RendererImpl::RendererImpl(World::Ptr w, uvec2 sz) : _w(w), _initSz(sz) {
    //SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(_initSz.x(), _initSz.y(), "t r i v o x");
    if (rlGetVersion() != RL_OPENGL_43)
        TraceLog(LOG_FATAL, "GL: TRIVOX requires a raylib OpenGL 4.3 build");
    Image icon = LoadImageFromMemory(".png", res_icon, res_icon_len);
    SetWindowIcon(icon);
    UnloadImage(icon);
    _resetCamPos();
    const std::string cameraValue = getEnvironmentValue("TRIVOX_CAMERA");
    Vector3 position = {};
    Vector3 target = {};
#if defined(_WIN32)
    const int parsedCameraValues = sscanf_s(cameraValue.c_str(), "%f,%f,%f,%f,%f,%f",
                                            &position.x, &position.y, &position.z,
                                            &target.x, &target.y, &target.z);
#else
    const int parsedCameraValues = std::sscanf(cameraValue.c_str(), "%f,%f,%f,%f,%f,%f",
                                               &position.x, &position.y, &position.z,
                                               &target.x, &target.y, &target.z);
#endif
    if (parsedCameraValues == 6) {
        const bool finiteCamera = std::isfinite(position.x) && std::isfinite(position.y) &&
                                  std::isfinite(position.z) && std::isfinite(target.x) &&
                                  std::isfinite(target.y) && std::isfinite(target.z);
        if (finiteCamera) {
            _cam.position = position;
            _cam.target = target;
        }
    }
    const std::string renderScaleValue = getEnvironmentValue("TRIVOX_RENDER_SCALE");
    if (!renderScaleValue.empty()) {
        char *end = nullptr;
        const float requestedScale = std::strtof(renderScaleValue.c_str(), &end);
        if (end != renderScaleValue.c_str() && *end == '\0' && std::isfinite(requestedScale))
            _scale = std::clamp(requestedScale, 0.0625f, 1.0f);
    }
    SetTargetFPS(120);
    SetExitKey(KEY_F4);

    _raytraceShader = LoadShaderFromMemory(nullptr, (const char *)res_raytrace_frag);
    _resCascShader = LoadShaderFromMemory(nullptr, (const char *)res_res_casc_frag);
    _pathtraceShader = LoadShaderFromMemory(nullptr, (const char *)res_pathtrace_frag);
    _pathtraceDisplayShader = LoadShaderFromMemory(nullptr, (const char *)res_pathtrace_display_frag);
    requireCustomShader(_raytraceShader, "radiance resolve");
    requireCustomShader(_resCascShader, "radiance cascade");
    requireCustomShader(_pathtraceShader, "path trace");
    requireCustomShader(_pathtraceDisplayShader, "path display");
    _winSz = Vector2{(float)_initSz.x(), (float)_initSz.y()};
    _baseWinSz = _winSz;
    _updateShaderSize();
}

RendererImpl::~RendererImpl() {
    _unloadRenderTargets();
    UnloadShader(_raytraceShader);
    UnloadShader(_resCascShader);
    UnloadShader(_pathtraceShader);
    UnloadShader(_pathtraceDisplayShader);
    CloseWindow();
}

Vector2 RendererImpl::getResCascTexSz() {
    int lvlside = res_casc_lvl_0_res * (1 << res_casc_n_probe_extra_lvls) * 8;
    int w = lvlside * (TRIVOX_MAX_LVL - TRIVOX_MIN_LVL + 1);
    int h = lvlside * 8 * (1 << res_casc_n_probe_extra_lvls);
    return {(float)w/* * TRIVOX_MAX_ROOMS*/, (float)h};
}

void RendererImpl::_unloadRenderTargets() {
    if (_backTex.id != 0) UnloadRenderTexture(_backTex);
    if (_frontTex.id != 0) UnloadRenderTexture(_frontTex);
    if (_traceTex.id != 0) UnloadRenderTexture(_traceTex);
    for (auto &texture : _resCascTex) {
        if (texture.id != 0) UnloadRenderTexture(texture);
        texture = {};
    }
    for (auto &texture : _pathtraceAccumTex) {
        if (texture.id != 0) UnloadRenderTexture(texture);
        texture = {};
    }
    _backTex = {};
    _frontTex = {};
    _traceTex = {};
}

void RendererImpl::_resetPathtraceAccumulation() {
    for (auto &texture : _pathtraceAccumTex) {
        if (texture.id == 0) continue;
        BeginTextureMode(texture);
        ClearBackground(BLANK);
        EndTextureMode();
    }
    _activePathtraceTex = 0;
    _pathtraceSampleCount = 0;
    _pathtraceResetPending = false;
}

void RendererImpl::_updateShaderSize() {
    _unloadRenderTargets();
    const int scaledWidth = std::max(1, (int)std::lround(_winSz.x * _scale));
    const int scaledHeight = std::max(1, (int)std::lround(_winSz.y * _scale));
    _sclWinSz = Vector2{(float)scaledWidth, (float)scaledHeight};
    SetShaderValue(_raytraceShader, GetShaderLocation(_raytraceShader, "RESOLUTION"), &_sclWinSz, SHADER_UNIFORM_VEC2);
    SetShaderValue(_pathtraceShader, GetShaderLocation(_pathtraceShader, "RESOLUTION"), &_sclWinSz, SHADER_UNIFORM_VEC2);
    _backTex = LoadRenderTexture((int)_sclWinSz.x, (int)_sclWinSz.y);
    _frontTex = LoadRenderTexture((int)_sclWinSz.x, (int)_sclWinSz.y);
    _traceTex = LoadRenderTexture((int)_sclWinSz.x, (int)_sclWinSz.y);
    if (!IsRenderTextureValid(_backTex) || !IsRenderTextureValid(_frontTex) ||
        !IsRenderTextureValid(_traceTex))
        TraceLog(LOG_FATAL, "FBO: Unable to create screen render targets");
    SetTextureFilter(_traceTex.texture, TEXTURE_FILTER_BILINEAR);
    BeginTextureMode(_backTex);
    ClearBackground(BLACK);
    EndTextureMode();
    BeginTextureMode(_frontTex);
    ClearBackground(BLACK);
    EndTextureMode();
    auto rctsz = getResCascTexSz();
    for (auto &texture : _resCascTex) {
        texture = loadColorRenderTexture((int)rctsz.x, (int)rctsz.y,
                                         PIXELFORMAT_UNCOMPRESSED_R16G16B16A16);
        if (texture.id == 0) {
            TraceLog(LOG_WARNING, "FBO: Falling back to RGBA32F cascade storage");
            texture = loadColorRenderTexture((int)rctsz.x, (int)rctsz.y,
                                             PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
        }
        if (texture.id == 0) {
            TraceLog(LOG_WARNING, "FBO: Falling back to RGBA8 cascade storage");
            texture = loadColorRenderTexture((int)rctsz.x, (int)rctsz.y,
                                             PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        }
        if (texture.id == 0)
            TraceLog(LOG_FATAL, "FBO: Unable to create radiance-cascade storage");
        SetTextureFilter(texture.texture, TEXTURE_FILTER_BILINEAR);
    }
    _pathtraceAvailable = true;
    for (auto &texture : _pathtraceAccumTex) {
        texture = loadColorRenderTexture((int)_sclWinSz.x, (int)_sclWinSz.y,
                                         PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
        if (texture.id == 0) {
            TraceLog(LOG_WARNING, "FBO: Falling back to RGBA16F path accumulation");
            texture = loadColorRenderTexture((int)_sclWinSz.x, (int)_sclWinSz.y,
                                             PIXELFORMAT_UNCOMPRESSED_R16G16B16A16);
        }
        _pathtraceAvailable = _pathtraceAvailable && texture.id != 0;
        if (texture.id != 0)
            SetTextureFilter(texture.texture, TEXTURE_FILTER_BILINEAR);
    }
    if (!_pathtraceAvailable) {
        TraceLog(LOG_WARNING, "PT: Disabled because float accumulation buffers are unavailable");
        for (auto &texture : _pathtraceAccumTex) {
            if (texture.id != 0) UnloadRenderTexture(texture);
            texture = {};
        }
        if (_renderMode == RenderMode::PathTrace)
            _renderMode = RenderMode::Cascades;
    }
    _activeResCascTex = 0;
    _resetPathtraceAccumulation();
    _lastReszTime = GetTime();
}

void RendererImpl::_resetCamPos() {
    _cam.position = {4.0f, 3.2f, -4.0f};
    _cam.target = {4.0f, 2.8f, 4.2f};
    _cam.up = {0.0f, 1.0f, 0.0f};
    _cam.fovy = 70.0f;
    _cam.projection = CAMERA_PERSPECTIVE;
}

void RendererImpl::_input() {
    const Vector3 previousPosition = _cam.position;
    const Vector3 previousTarget = _cam.target;

    bool shift = IsKeyDown(KEY_LEFT_SHIFT);
    float speed = SPEED * (shift ? FAST_COEFF : 1.0f);

    float forwards = ((IsKeyDown(KEY_W)) - (IsKeyDown(KEY_S))) * speed;
    float sideways = ((IsKeyDown(KEY_D)) - (IsKeyDown(KEY_A))) * speed;
    float vertical = ((IsKeyDown(KEY_E)) - (IsKeyDown(KEY_Q))) * speed;

    Vector2 mdelta = GetMouseDelta();
    bool canRot = (_time - _lastReszTime > RESIZE_CD) && (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsCursorHidden());
    Vector3 rot = canRot ? (Vector3{mdelta.x, mdelta.y, 0.0f} * 0.2f) : Vector3Zero();

    Vector3 dir = Vector3Normalize(Vector3{_cam.target.x - _cam.position.x,
                                           _cam.target.y - _cam.position.y,
                                           _cam.target.z - _cam.position.z});
    Vector3 flatFwd = GetCameraForward(&_cam);
    flatFwd.y = 0;
    flatFwd = Vector3Normalize(flatFwd);
    float pitch = atan2(Vector3DotProduct(dir, _cam.up), Vector3DotProduct(dir, flatFwd));
    Vector3 camUp = Vector3RotateByAxisAngle(Vector3{0, 0, vertical},
                                             Vector3{0, 1.0f, 0}, -pitch);

    UpdateCameraPro(&_cam, camUp, rot, 0.0f);
    CameraMoveForward(&_cam, forwards, false);
    CameraMoveRight(&_cam, sideways, false);

    if (IsKeyPressed(KEY_F) || IsKeyPressed(KEY_F11) || (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))) {
        if (!IsWindowFullscreen()) {
            auto mid = GetCurrentMonitor();
            auto newWinSz = Vector2{(float)GetMonitorWidth(mid), (float)GetMonitorHeight(mid)};
            float xCoeff = newWinSz.x / _winSz.x;
            float yCoeff = newWinSz.y / _winSz.y;
            _winSz = newWinSz;
            SetWindowSize(int(_winSz.x), int(_winSz.y));
        }
        ToggleFullscreen();
        if (!IsWindowFullscreen()) {
            auto newWinSz = _baseWinSz;
            float xCoeff = newWinSz.x / _winSz.x;
            float yCoeff = newWinSz.y / _winSz.y;
            _winSz = newWinSz;
            SetWindowSize(int(_winSz.x), int(_winSz.y));
        }
        _updateShaderSize();
    } else if (IsWindowResized()) {
        _winSz = Vector2{(float)GetScreenWidth(), (float)GetScreenHeight()};
        if (!IsWindowFullscreen())
            _baseWinSz = _winSz;
        _updateShaderSize();
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && IsGestureDetected(GESTURE_DOUBLETAP))
        DisableCursor();

    if (IsKeyPressed(KEY_LEFT_CONTROL) || IsKeyPressed(KEY_ESCAPE))
        EnableCursor();

    if (IsKeyPressed(KEY_G)) {
        _drawGrids = !_drawGrids;
        if (!_drawGrids) {
            BeginTextureMode(_backTex);
            ClearBackground(BLACK);
            EndTextureMode();
            BeginTextureMode(_frontTex);
            ClearBackground(BLACK);
            EndTextureMode();
        }
    }

    if (IsKeyPressed(KEY_C))
        _drawCasc = !_drawCasc;

    if (IsKeyPressed(KEY_P)) {
        if (_renderMode == RenderMode::PathTrace) {
            _renderMode = RenderMode::Cascades;
        } else if (_pathtraceAvailable) {
            _renderMode = RenderMode::PathTrace;
            _pathtraceResetPending = true;
        } else {
            TraceLog(LOG_WARNING, "PT: Float accumulation buffers are unavailable");
        }
    }

    if (IsKeyPressed(KEY_R))
        _pathtraceResetPending = true;

    float newScale = _scale;
    if (IsKeyPressed(KEY_EQUAL))
        newScale *= 2.0f;
    if (IsKeyPressed(KEY_MINUS))
        newScale /= 2.0f;
    newScale = std::clamp(newScale, 0.0625f, 1.0f);
    if (newScale != _scale) {
        _scale = newScale;
        _updateShaderSize();
    }

    if (Vector3Distance(previousPosition, _cam.position) > 1e-6f ||
        Vector3Distance(previousTarget, _cam.target) > 1e-6f)
        _pathtraceResetPending = true;
}

void drawRoomGrid(vec3 A, vec3 B, vec3 C, vec3 D, int n1, int n2, vec3 campos,
                  bool front) {
    auto center = A + (B - A) / 2 + (D - A) / 2;
    auto tocam = (campos - center).normalized();
    auto normal = ((B - A).cross((C - A))).normalized();
    if (front == (normal.dot(tocam) > 0)) {
        for (int i = 0; i <= n1; ++i) {
            auto st1 = i * (D - A) / ((float)n1);
            DrawLine3D(toray3(A + st1), toray3(B + st1), GRAY);
        }
        for (int i = 0; i <= n2; ++i) {
            auto st2 = i * (B - A) / ((float)n2);
            DrawLine3D(toray3(A + st2), toray3(D + st2), GRAY);
        }
    }
}

void RendererImpl::_drawRoomGrids(Vector3 campos, bool front) {
    auto ecampos = fromray3(campos);

    for (int i = 0; i < _w->_state.roomRefs.count(); ++i) {

        auto &rr = _w->_state.roomRefs.at(i);
        auto &room = _w->_state.rooms.at(_w->_state.roomRefs.at(i).idx - 1);
        Room rrr = room;

        vec3 roomHalfSz = room.size * 0.5f;
        mat4 roomMat = rr.matrix();
        mat3 roomRot = roomMat.ROTMAT;
        vec3 roomCenter = roomMat.POSVEC + roomMat.ROTMAT * roomHalfSz;

        vec3 A = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{1, -1, 1}));
        vec3 B = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{1, -1, -1}));
        vec3 C = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{-1, -1, 1}));
        vec3 D = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{-1, -1, -1}));
        vec3 E = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{1, 1, 1}));
        vec3 F = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{1, 1, -1}));
        vec3 G = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{-1, 1, 1}));
        vec3 H = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{-1, 1, -1}));

        drawRoomGrid(A, B, D, C, room.size.x(), room.size.z(), ecampos, front);
        drawRoomGrid(F, B, A, E, room.size.z(), room.size.y(), ecampos, front);
        drawRoomGrid(C, D, H, G, room.size.y(), room.size.z(), ecampos, front);
        drawRoomGrid(B, F, H, D, room.size.x(), room.size.y(), ecampos, front);
        drawRoomGrid(E, A, C, G, room.size.x(), room.size.y(), ecampos, front);
        drawRoomGrid(G, H, F, E, room.size.x(), room.size.z(), ecampos, front);
    }
}

void RendererImpl::startRender() {
    const std::string capturePath = getEnvironmentValue("TRIVOX_CAPTURE_PATH");
    const std::string captureFrameValue = getEnvironmentValue("TRIVOX_CAPTURE_FRAME");
    const bool captureTargets = !getEnvironmentValue("TRIVOX_CAPTURE_TARGETS").empty();
    const bool freezeWorld = !getEnvironmentValue("TRIVOX_FREEZE_WORLD").empty();
    const int captureFrame = captureFrameValue.empty() ? 3 : std::max(1, std::atoi(captureFrameValue.c_str()));
    const int pathtraceCaptureSamples = getEnvironmentInt("TRIVOX_PT_CAPTURE_SPP", 0, 0, 1000000);
    _pathtraceMaxBounces = getEnvironmentInt("TRIVOX_PT_BOUNCES", 6, 1, 12);
    _pathtraceSamplesPerFrame = getEnvironmentInt("TRIVOX_PT_SPP_PER_FRAME", 1, 1, 8);
    const std::string requestedRenderer = getEnvironmentValue("TRIVOX_RENDERER");
    if (_pathtraceAvailable &&
        (requestedRenderer == "pathtrace" || requestedRenderer == "path" || requestedRenderer == "pt")) {
        _renderMode = RenderMode::PathTrace;
        _pathtraceResetPending = true;
    } else if (!requestedRenderer.empty() && !_pathtraceAvailable) {
        TraceLog(LOG_WARNING, "PT: Requested renderer is unavailable; using cascades");
    }
    int frameNumber = 0;

    _w->update(0.0f);

    const unsigned int vertexBufferSize =
        (unsigned int)(_w->_state.vertices.count() * sizeof(Vertex));
    const unsigned int shapeBufferSize =
        (unsigned int)(_w->_state.shapes.count() * sizeof(Shape));
    auto ssboVerts = rlLoadShaderBuffer(vertexBufferSize, _w->_state.vertices.data(), RL_DYNAMIC_DRAW);
    auto ssboShapes = rlLoadShaderBuffer(shapeBufferSize, _w->_state.shapes.data(), RL_STATIC_DRAW);
    if (ssboVerts == 0 || ssboShapes == 0)
        TraceLog(LOG_FATAL, "SSBO: Unable to allocate scene buffers");
    rlBindShaderBuffer(ssboVerts, 0);
    rlBindShaderBuffer(ssboShapes, 1);
    const int shapeCount = (int)_w->_state.shapes.count();
    int lightCount = 0;
    for (int i = 0; i < shapeCount; ++i) {
        const Shape &shape = _w->_state.shapes.at(i);
        if (shape.materialIdx == SHAPE_MATERIAL_EMISSIVE && shape.type == ShapeType::SPHERE)
            ++lightCount;
    }

    while (!WindowShouldClose()) {
        if (_renderMode == RenderMode::Cascades && !freezeWorld)
            _w->update(std::min(GetFrameTime(), 0.1f));

        _time = GetTime();

        Matrix matView = GetCameraViewMatrix(&_cam);
        Matrix matProj = GetCameraProjectionMatrix(&_cam, 1.0);
        Matrix mvp = MatrixInvert(MatrixMultiply(matView, matProj));

        if (_renderMode == RenderMode::Cascades && _drawGrids) {
            BeginTextureMode(_backTex);
            ClearBackground(BLACK);
            BeginMode3D(_cam);
            _drawRoomGrids(_cam.position);
            EndMode3D();
            EndTextureMode();

            BeginTextureMode(_frontTex);
            ClearBackground(BLACK);
            BeginMode3D(_cam);
            _drawRoomGrids(_cam.position, true);
            EndMode3D();
            EndTextureMode();
        }

        rlUpdateShaderBuffer(ssboVerts, _w->_state.vertices.data(), vertexBufferSize, 0);

        RenderTexture2D *cascadeTexture = nullptr;
        if (_renderMode == RenderMode::PathTrace) {
            if (_pathtraceResetPending)
                _resetPathtraceAccumulation();

            SetShaderValueMatrix(_pathtraceShader, GetShaderLocation(_pathtraceShader, "CAM_MVP"), mvp);
            SetShaderValue(_pathtraceShader, GetShaderLocation(_pathtraceShader, "CAM_POS"), &_cam.position, SHADER_UNIFORM_VEC3);
            SetShaderValue(_pathtraceShader, GetShaderLocation(_pathtraceShader, "N_SHAPES"), &shapeCount, SHADER_UNIFORM_INT);
            SetShaderValue(_pathtraceShader, GetShaderLocation(_pathtraceShader, "N_LIGHTS"), &lightCount, SHADER_UNIFORM_INT);
            SetShaderValue(_pathtraceShader, GetShaderLocation(_pathtraceShader, "SAMPLE_INDEX"), &_pathtraceSampleCount, SHADER_UNIFORM_INT);
            SetShaderValue(_pathtraceShader, GetShaderLocation(_pathtraceShader, "SPP_PER_FRAME"), &_pathtraceSamplesPerFrame, SHADER_UNIFORM_INT);
            SetShaderValue(_pathtraceShader, GetShaderLocation(_pathtraceShader, "MAX_BOUNCES"), &_pathtraceMaxBounces, SHADER_UNIFORM_INT);

            const int writeTexture = 1 - _activePathtraceTex;
            BeginTextureMode(_pathtraceAccumTex[writeTexture]);
            BeginShaderMode(_pathtraceShader);
            DrawTextureRec(_pathtraceAccumTex[_activePathtraceTex].texture,
                           Rectangle{0, 0,
                                     (float)_pathtraceAccumTex[_activePathtraceTex].texture.width,
                                     (float)-_pathtraceAccumTex[_activePathtraceTex].texture.height},
                           Vector2Zero(), WHITE);
            EndShaderMode();
            EndTextureMode();
            _activePathtraceTex = writeTexture;
            _pathtraceSampleCount += _pathtraceSamplesPerFrame;

            const float exposure = 1.0f;
            SetShaderValue(_pathtraceDisplayShader,
                           GetShaderLocation(_pathtraceDisplayShader, "EXPOSURE"),
                           &exposure, SHADER_UNIFORM_FLOAT);
            BeginTextureMode(_traceTex);
            ClearBackground(BLACK);
            BeginShaderMode(_pathtraceDisplayShader);
            DrawTexturePro(_pathtraceAccumTex[_activePathtraceTex].texture,
                           Rectangle{0, 0,
                                     (float)_pathtraceAccumTex[_activePathtraceTex].texture.width,
                                     (float)-_pathtraceAccumTex[_activePathtraceTex].texture.height},
                           Rectangle{0, 0, _sclWinSz.x, _sclWinSz.y},
                           Vector2Zero(), 0, WHITE);
            EndShaderMode();
            const std::string status = "PT " + std::to_string(_pathtraceSampleCount) +
                                       " spp  " + std::to_string(GetFPS()) + " fps";
            DrawText(status.c_str(), 10, 10, 10, RED);
            EndTextureMode();
        } else {
            SetShaderValue(_resCascShader, GetShaderLocation(_resCascShader, "N_PROBE_EXTRA_LVLS"), &res_casc_n_probe_extra_lvls, SHADER_UNIFORM_INT);
            SetShaderValue(_resCascShader, GetShaderLocation(_resCascShader, "LVL_0_RES"), &res_casc_lvl_0_res, SHADER_UNIFORM_INT);
            SetShaderValue(_resCascShader, GetShaderLocation(_resCascShader, "N_SHAPES"), &shapeCount, SHADER_UNIFORM_INT);
            BeginTextureMode(_resCascTex[0]);
            ClearBackground(BLANK);
            EndTextureMode();
            _activeResCascTex = 0;
            for (int i = 0; i < res_casc_n_iterations; ++i) {
                SetShaderValue(_resCascShader, GetShaderLocation(_resCascShader, "ITERATION"), &i, SHADER_UNIFORM_INT);
                const int writeTexture = 1 - _activeResCascTex;
                BeginTextureMode(_resCascTex[writeTexture]);
                ClearBackground(BLANK);
                BeginShaderMode(_resCascShader);
                DrawTextureRec(_resCascTex[_activeResCascTex].texture,
                               Rectangle{0, 0,
                                         (float)_resCascTex[_activeResCascTex].texture.width,
                                         (float)-_resCascTex[_activeResCascTex].texture.height},
                               Vector2Zero(), WHITE);
                EndShaderMode();
                EndTextureMode();
                _activeResCascTex = writeTexture;
            }
            cascadeTexture = &_resCascTex[_activeResCascTex];

            SetShaderValueMatrix(_raytraceShader, GetShaderLocation(_raytraceShader, "CAM_MVP"), mvp);
            SetShaderValue(_raytraceShader, GetShaderLocation(_raytraceShader, "CAM_POS"), &_cam.position, SHADER_UNIFORM_VEC3);
            SetShaderValue(_raytraceShader, GetShaderLocation(_raytraceShader, "N_PROBE_EXTRA_LVLS"), &res_casc_n_probe_extra_lvls, SHADER_UNIFORM_INT);
            SetShaderValue(_raytraceShader, GetShaderLocation(_raytraceShader, "LVL_0_RES"), &res_casc_lvl_0_res, SHADER_UNIFORM_INT);
            SetShaderValue(_raytraceShader, GetShaderLocation(_raytraceShader, "N_SHAPES"), &shapeCount, SHADER_UNIFORM_INT);

            BeginTextureMode(_traceTex);
            ClearBackground(BLACK);
            BeginShaderMode(_raytraceShader);
            rlEnableShader(_raytraceShader.id);
            rlSetUniformSampler(GetShaderLocation(_raytraceShader, "texture1"), _frontTex.texture.id);
            rlSetUniformSampler(GetShaderLocation(_raytraceShader, "texture2"), cascadeTexture->texture.id);
            DrawTexturePro(_backTex.texture,
                           Rectangle{0, 0, (float)_backTex.texture.width, (float)-_backTex.texture.height},
                           Rectangle{0, 0, _sclWinSz.x, _sclWinSz.y},
                           Vector2Zero(), 0, WHITE);
            EndShaderMode();
            const std::string status = "RC  " + std::to_string(GetFPS()) + " fps";
            DrawText(status.c_str(), 10, 10, 10, RED);
            EndTextureMode();
        }

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(_traceTex.texture,
            Rectangle{0, 0, (float)_traceTex.texture.width, (float)-_traceTex.texture.height},
            Rectangle{0, 0, _winSz.x, _winSz.y}, Vector2Zero(), 0, WHITE);
        if (_renderMode == RenderMode::Cascades && _drawCasc && cascadeTexture != nullptr) {
            DrawTextureRec(cascadeTexture->texture,
                Rectangle{0, 0, (float)cascadeTexture->texture.width, (float)-cascadeTexture->texture.height},
                Vector2{0, 0}, WHITE);
        }
        EndDrawing();

        ++frameNumber;
        const bool usePathtraceTarget = _renderMode == RenderMode::PathTrace &&
                                        pathtraceCaptureSamples > 0;
        const bool reachedPathtraceTarget = usePathtraceTarget &&
                                            _pathtraceSampleCount >= pathtraceCaptureSamples;
        const bool reachedFrameTarget = !usePathtraceTarget && frameNumber >= captureFrame;
        if (!capturePath.empty() && (reachedPathtraceTarget || reachedFrameTarget)) {
            exportRenderTexture(_traceTex, capturePath.c_str());
            if (captureTargets) {
                exportRenderTexture(_backTex, "capture-back.png");
                exportRenderTexture(_frontTex, "capture-front.png");
                exportRenderTexture(_traceTex, "capture-trace.png");
                if (cascadeTexture != nullptr)
                    exportRenderTexture(*cascadeTexture, "capture-cascade.png");
            }
            break;
        }

        _input();
    }

    rlUnloadShaderBuffer(ssboVerts);
    rlUnloadShaderBuffer(ssboShapes);
}

Renderer::Ptr Renderer::create(World::Ptr w, uvec2 sz) {
    return std::shared_ptr<Renderer>(new RendererImpl(w, sz));
}
