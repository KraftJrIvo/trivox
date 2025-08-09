#include "render.h"
#include "raymath.h"
#include "rcamera.h"
#include "rlgl.h"

#define SPEED 0.1f
#define FAST_COEFF 5.0f
#define RESIZE_CD 0.25f

extern "C" const unsigned char res_icon[];
extern "C" const size_t res_icon_len;

extern "C" const unsigned char res_raytrace_frag[];

class RendererImpl : public Renderer {
    World::Ptr _w;
    Camera _cam;
    uvec2 _initSz;
    Vector2 _winSz, _baseWinSz;
    float _time, _lastReszTime;
    Shader _shader;
    RenderTexture2D _backTex, _frontTex;
    bool _drawGrids = true;

    void _resetCamPos();
    void _updateShaderSize();
    void _input();
    void _drawRoomGrids(Vector3 campos, bool front = false);

  public:
    RendererImpl(World::Ptr w, uvec2 sz);
    virtual void startRender() override;
};

RendererImpl::RendererImpl(World::Ptr w, uvec2 sz) : _w(w), _initSz(sz) {
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(_initSz.x(), _initSz.y(), "t r i v o x");
    SetWindowIcon(LoadImageFromMemory(".png", res_icon, res_icon_len));
    _resetCamPos();
    SetTargetFPS(60);
    SetExitKey(KEY_F4);

    _shader = LoadShaderFromMemory(nullptr, (const char *)res_raytrace_frag);
    _winSz = Vector2{(float)_initSz.x(), (float)_initSz.y()};
    _baseWinSz = _winSz;
    _updateShaderSize();
}

void RendererImpl::_updateShaderSize() {
    SetShaderValue(_shader, GetShaderLocation(_shader, "RESOLUTION"), &_winSz,
                   SHADER_ATTRIB_VEC2);
    Image imBlank = GenImageColor(_winSz.x, _winSz.y, BLANK);
    _backTex = LoadRenderTexture(_winSz.x, _winSz.y);
    _frontTex = LoadRenderTexture(_winSz.x, _winSz.y);
    _lastReszTime = GetTime();
}

void RendererImpl::_resetCamPos() {
    _cam.position = {0.0f, 2.0f, -8.0f};
    _cam.target = {0.0f, 0.0f, 1.0f};
    _cam.up = {0.0f, 1.0f, 0.0f};
    _cam.fovy = 90.0f;
    _cam.projection = CAMERA_PERSPECTIVE;
}

void RendererImpl::_input() {

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
    auto ssboVerts = rlLoadShaderBuffer(_w->_state.vertices.size(), _w->_state.vertices.data(), RL_DYNAMIC_DRAW);
    auto ssboShapes = rlLoadShaderBuffer(_w->_state.shapes.size(), _w->_state.shapes.data(), RL_DYNAMIC_DRAW);
    auto ssboRooms = rlLoadShaderBuffer(_w->_state.rooms.size(), _w->_state.rooms.data(), RL_DYNAMIC_DRAW);
    auto ssboRoomRefs = rlLoadShaderBuffer(_w->_state.roomRefs.size(), _w->_state.roomRefs.data(), RL_DYNAMIC_DRAW);
    auto ssboCells = rlLoadShaderBuffer(_w->_cells.size(), _w->_cells.data(), RL_DYNAMIC_DRAW);
    rlBindShaderBuffer(ssboVerts, 0);
    rlBindShaderBuffer(ssboShapes, 1);
    rlBindShaderBuffer(ssboRooms, 2);
    rlBindShaderBuffer(ssboRoomRefs, 3);
    rlBindShaderBuffer(ssboCells, 4);

    while (!WindowShouldClose()) {
        static bool once = true;
        if (once && GetFrameTime() < 0.1f)
            //_w->update(GetMouseWheelMove() * 0.1f);
            _w->update(GetFrameTime());
        //once = false;

        _time = GetTime();
        SetShaderValue(_shader, GetShaderLocation(_shader, "TIME"), &_time,
                       SHADER_ATTRIB_FLOAT);

        Vector3 lookdir = Vector3Normalize(Vector3{
            _cam.target.x - _cam.position.x, 
            _cam.target.y - _cam.position.y,
            _cam.target.z - _cam.position.z
        });

        Matrix matView = GetCameraViewMatrix(&_cam);
        Matrix matProj = GetCameraProjectionMatrix(&_cam, 1.0);
        Matrix mvp = MatrixInvert(MatrixMultiply(matView, matProj));

        if (_drawGrids) {
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

        rlUpdateShaderBuffer(ssboVerts, _w->_state.vertices.data(), _w->_state.vertices.size(), 0);
        rlUpdateShaderBuffer(ssboShapes, _w->_state.shapes.data(), _w->_state.shapes.size(), 0);
        rlUpdateShaderBuffer(ssboRooms, _w->_state.rooms.data(), _w->_state.rooms.size(), 0);
        rlUpdateShaderBuffer(ssboRoomRefs, _w->_state.roomRefs.data(), _w->_state.roomRefs.size(), 0);
        rlUpdateShaderBuffer(ssboCells, _w->_cells.data(), _w->_cells.size(), 0);

        SetShaderValueMatrix(_shader, GetShaderLocation(_shader, "CAM_MVP"), mvp);
        SetShaderValue(_shader, GetShaderLocation(_shader, "CAM_FOV"), &_cam.fovy, SHADER_ATTRIB_FLOAT);
        SetShaderValue(_shader, GetShaderLocation(_shader, "CAM_POS"), &_cam.position, SHADER_ATTRIB_VEC3);
        SetShaderValue(_shader, GetShaderLocation(_shader, "TIME"), &_time, SHADER_ATTRIB_FLOAT);

        BeginDrawing();
        BeginShaderMode(_shader);
        rlEnableShader(_shader.id);
        rlSetUniformSampler(GetShaderLocation(_shader, "texture1"), _frontTex.texture.id);
        DrawTextureRec(_backTex.texture,
                       Rectangle{0, 0, (float)_backTex.texture.width, (float)-_backTex.texture.height},
                       Vector2{0, 0}, WHITE);
        EndShaderMode();
        EndDrawing();

        _input();
    }
}

Renderer::Ptr Renderer::create(World::Ptr w, uvec2 sz) {
    return std::shared_ptr<Renderer>(new RendererImpl(w, sz));
}