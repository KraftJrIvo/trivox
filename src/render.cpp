#include "raylib.h"
#include "types.hpp"
#include "raymath.h"
#include "rcamera.h"
#include "rlgl.h"

#define WIN_WIDTH 768
#define WIN_HEIGHT 512

#define SPEED 0.1f
#define FAST_COEFF 5.0f
#define RESIZE_CD 0.25f

extern "C" const unsigned char res_icon[];
extern "C" const size_t        res_icon_len;

extern "C" const unsigned char res_raytrace_frag[];

void Renderer::_updateShaderSize() {
    SetShaderValue(_shader, GetShaderLocation(_shader, "RESOLUTION"), &_winSz, SHADER_ATTRIB_VEC2);
    Image imBlank = GenImageColor(_winSz.x, _winSz.y, BLANK);
    _backTex = LoadRenderTexture(_winSz.x, _winSz.y);
    _frontTex = LoadRenderTexture(_winSz.x, _winSz.y);
    _lastReszTime = GetTime();
}

void Renderer::_resetCamPos() {
    _cam.position = { 0.0f, 2.0f, 8.0f };
    _cam.target = { 0.0f, 0.0f, -1.0f };
    _cam.up = { 0.0f, 1.0f, 0.0f };
    _cam.fovy = 90.0f;      
    _cam.projection = CAMERA_PERSPECTIVE;
}

Renderer::Renderer(World& w) :
    _w(w)
{ 
    // /SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WIN_WIDTH, WIN_HEIGHT, "t r i v o x");
    SetWindowIcon(LoadImageFromMemory(".png", res_icon, res_icon_len));
    _resetCamPos();
    SetTargetFPS(60);    
    SetExitKey(KEY_F4);

    _shader = LoadShaderFromMemory(nullptr, (const char*)res_raytrace_frag);
    _winSz = Vector2{WIN_WIDTH, WIN_HEIGHT};
    _baseWinSz = _winSz;
    _updateShaderSize();
}

void Renderer::_input() {

    bool shift = IsKeyDown(KEY_LEFT_SHIFT);
    float speed = SPEED * (shift ? FAST_COEFF : 1.0f);

    float forwards = ((IsKeyDown(KEY_W)) - (IsKeyDown(KEY_S))) * speed;
    float sideways = ((IsKeyDown(KEY_D)) - (IsKeyDown(KEY_A))) * speed;
    float vertical = ((IsKeyDown(KEY_E)) - (IsKeyDown(KEY_Q))) * speed;

    Vector2 mdelta = GetMouseDelta();
    bool canRot = (_time - _lastReszTime > RESIZE_CD) && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    Vector3 rot = canRot ? (Vector3{mdelta.x, mdelta.y, 0.0f} * 0.2f) : Vector3Zero();

    Vector3 dir = Vector3Normalize(Vector3{_cam.target.x - _cam.position.x, _cam.target.y - _cam.position.y, _cam.target.z - _cam.position.z});
    Vector3 flatFwd = GetCameraForward(&_cam); flatFwd.y = 0; flatFwd = Vector3Normalize(flatFwd);
    float pitch = atan2(Vector3DotProduct(dir, _cam.up), Vector3DotProduct(dir, flatFwd));
    Vector3 camUp = Vector3RotateByAxisAngle(Vector3{ 0, 0, vertical }, Vector3{ 0, 1.0f, 0 }, -pitch);

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
            EnableCursor();
            auto newWinSz = _baseWinSz;
            float xCoeff = newWinSz.x / _winSz.x;       
            float yCoeff = newWinSz.y / _winSz.y;       
            _winSz = newWinSz;
            SetWindowSize(int(_winSz.x), int(_winSz.y));
        }
        _updateShaderSize();
    } else if (IsWindowResized()) {
        _winSz = Vector2{(float)GetScreenWidth(), (float)GetScreenHeight()};
        _baseWinSz = _winSz;
        _updateShaderSize();
    }
}

void Renderer::startRender()
{
    while (!WindowShouldClose()) 
    {
        _time = GetTime();
        SetShaderValue(_shader, GetShaderLocation(_shader, "TIME"), &_time, SHADER_ATTRIB_FLOAT);

        Vector3 lookdir = Vector3Normalize(Vector3{_cam.target.x - _cam.position.x, _cam.target.y - _cam.position.y, _cam.target.z - _cam.position.z});
        
        Matrix matView = GetCameraViewMatrix(&_cam);
        Matrix matProj = GetCameraProjectionMatrix(&_cam, 1.0);
        Matrix mvp = MatrixInvert(MatrixMultiply(matView, matProj));

        BeginTextureMode(_backTex);
            ClearBackground(BLACK);
            BeginMode3D(_cam);
                _w.drawRoomGrids(_cam.position);
            EndMode3D();
        EndTextureMode();

        BeginTextureMode(_frontTex);
            ClearBackground(BLACK);
            BeginMode3D(_cam);
                _w.drawRoomGrids(_cam.position, true);
            EndMode3D();
        EndTextureMode();

        BeginDrawing();
            SetShaderValueMatrix(_shader, GetShaderLocation(_shader, "CAM_MVP"), mvp);
            SetShaderValue(_shader, GetShaderLocation(_shader, "CAM_FOV"), &_cam.fovy, SHADER_ATTRIB_FLOAT);
            SetShaderValue(_shader, GetShaderLocation(_shader, "CAM_POS"), &_cam.position, SHADER_ATTRIB_VEC3);
            SetShaderValue(_shader, GetShaderLocation(_shader, "TIME"), &_time, SHADER_ATTRIB_FLOAT);
            BeginShaderMode(_shader);
                SetShaderValueTexture(_shader, GetShaderLocation(_shader, "texture1"), _frontTex.texture);
                DrawTextureRec(_backTex.texture, Rectangle{ 0, 0, (float)_backTex.texture.width, (float)-_backTex.texture.height }, (Vector2) { 0, 0 }, WHITE);
            EndShaderMode();
        EndDrawing();

        _input();
    }
}