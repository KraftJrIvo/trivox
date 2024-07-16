#include "renderer.h"

#include "raylib.h"
#include "raymath.h"
#include "rcamera.h"
#include "rlgl.h"

#include "vec_ops.h"

#include <cmath>
//#include <iostream>

using namespace trivox;

#define SPEED 0.1f
#define FAST_COEFF 5.0f
#define RESIZE_CD 0.25f

Renderer::Renderer(World& w, const RendererConfig& cfg, const Vector2& winSize, const std::string& winName) :
	_w(w),
    _cfg(cfg),
	_baseWindowSize(winSize),
	_windowSize(winSize),
	_windowName(winName),
    _pyramid(cfg.minMaxPow, cfg.radius)
{
    _redrawer = std::thread([&]() {
        _init();
        while (!WindowShouldClose()) {
            _render();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    	}
        CloseWindow();
        done = true;
    });
}

Renderer::~Renderer() {
    _redrawer.join();
}

void Renderer::_resetPlayerPos() {
    _cam.position = { 0.0f, 2.0f, 8.0f };
    _cam.target = { 0.0f, 0.0f, -1.0f };
    _cam.up = { 0.0f, 1.0f, 0.0f };
    _cam.fovy = 90.0f;      
    _cam.projection = CAMERA_PERSPECTIVE;
}

void Renderer::_init() {
    InitWindow((int)_windowSize.x, (int)_windowSize.y, _windowName.c_str());
    _resetPlayerPos();
    DisableCursor();
    SetTargetFPS(60);

    _shader = LoadShader(nullptr, "res/raytrace.frag");
    _updateShaderSize();
}

void Renderer::_updateShaderSize() {
    SetShaderValue(_shader, GetShaderLocation(_shader, "RESOLUTION"), &_windowSize, SHADER_ATTRIB_VEC2);
    Image imBlank = GenImageColor(_windowSize.x, _windowSize.y, BLANK);
    _shTex = LoadTextureFromImage(imBlank);
    _lastResizeTime = _time;
}

void AllocateMeshData(Mesh* mesh, int triangleCount)
{
    mesh->vertexCount = triangleCount * 3;
    mesh->triangleCount = triangleCount;

    mesh->vertices = (float*)MemAlloc(mesh->vertexCount * 3 * sizeof(float));
    mesh->texcoords = (float*)MemAlloc(mesh->vertexCount * 2 * sizeof(float));
    mesh->normals = (float*)MemAlloc(mesh->vertexCount * 3 * sizeof(float));
}

void Renderer::_render() {
    bool shift = IsKeyDown(KEY_LEFT_SHIFT);
    float speed = SPEED * (shift ? FAST_COEFF : 1.0f);

    float forwards = ((IsKeyDown(KEY_W)) - (IsKeyDown(KEY_S))) * speed;
    float sideways = ((IsKeyDown(KEY_D)) - (IsKeyDown(KEY_A))) * speed;
    float vertical = ((IsKeyDown(KEY_E)) - (IsKeyDown(KEY_Q))) * speed;

    Vector2 mdelta = GetMouseDelta();
    Vector3 rot = (_time - _lastResizeTime > RESIZE_CD) ? (Vector3{mdelta.x, mdelta.y, 0.0f} * 0.05f) : Vector3Zero();

    Vector3 dir = Vector3Normalize(Vector3{_cam.target.x - _cam.position.x, _cam.target.y - _cam.position.y, _cam.target.z - _cam.position.z});
    Vector3 flatFwd = GetCameraForward(&_cam); flatFwd.y = 0; flatFwd = Vector3Normalize(flatFwd);
    float pitch = atan2(Vector3DotProduct(dir, _cam.up), Vector3DotProduct(dir, flatFwd));
    Vector3 camUp = Vector3RotateByAxisAngle(Vector3{ 0, 0, vertical }, Vector3{ 0, 1.0f, 0 }, -pitch);

    UpdateCameraPro(&_cam, camUp, rot, 0.0f);
    CameraMoveForward(&_cam, forwards, false);
    CameraMoveRight(&_cam, sideways, false);

    _time = _w._time;
    SetShaderValue(_shader, GetShaderLocation(_shader, "TIME"), &_time, SHADER_ATTRIB_FLOAT);
    
    Matrix matView = GetCameraViewMatrix(&_cam);
    Matrix matProj = GetCameraProjectionMatrix(&_cam, 1.0);
    Matrix mvp = MatrixInvert(MatrixMultiply(matView, matProj));

    SetShaderValueMatrix(_shader, GetShaderLocation(_shader, "CAM_MVP"), mvp);
    SetShaderValue(_shader, GetShaderLocation(_shader, "CAM_FOV"), &_cam.fovy, SHADER_ATTRIB_FLOAT);
    SetShaderValue(_shader, GetShaderLocation(_shader, "CAM_POS"), &_cam.position, SHADER_ATTRIB_VEC3);

    SetShaderValue(_shader, GetShaderLocation(_shader, "TIME"), &_time, SHADER_ATTRIB_FLOAT);
    _light = { 10.0f * cos(_time), 10.0f, 10.0f * sin(_time)};
    SetShaderValue(_shader, GetShaderLocation(_shader, "LIGHT_POS"), &_light, SHADER_ATTRIB_VEC3);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    {
        BeginShaderMode(_shader);
        DrawTexture(_shTex, 0, 0, WHITE);
        EndShaderMode();
    }
    EndDrawing();

    if (IsKeyPressed(KEY_F)) {
        if (!IsWindowFullscreen()) {
            _windowSize = {(float)GetMonitorWidth(0), (float)GetMonitorHeight(0)};
            SetWindowSize(_windowSize.x, _windowSize.y);
        }
        ToggleFullscreen();
        if (!IsWindowFullscreen()) {
            _windowSize = _baseWindowSize;
            SetWindowSize(_windowSize.x, _windowSize.y);
        }
        _updateShaderSize();
    }
}