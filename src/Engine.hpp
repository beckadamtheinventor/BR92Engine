#pragma once

#include "Configs.hpp"
#include "Entity.hpp"
#include "MapData.hpp"
#include "Registries.hpp"
#include "imgui.h"
#include "raylib.h"
#include "ScriptEngine/ScriptBytecode.hpp"
#include "ScriptEngine/ScriptAssemblyCompiler.hpp"

#define PLAYER_SPEED 1.5f

extern EntityRenderer* GlobalEntityRenderer;

class BR92Engine {
    public:
    MainConfig* cfg=nullptr;
    ShaderConfig* scfg=nullptr;
    DevConfig* dcfg=nullptr;
    char* levelFileName=nullptr;
    Shader postShader;
    RenderTexture2D gameTexture;
    RenderTexture2D screenTexture;
    Camera3D camera;
    ImVec2 gameWindowPosition;
    Vector3 dev_lightPosition;
    float dev_lightValue;
    float dev_lightColor[3];
    float mouseSensitivity, playerSpeed, playerMomentumVertical;
    int renderScale, targetFps;
    unsigned int postVao;
    union {
        int _flags;
        struct {
            bool drawing_menus : 1;
            bool is_first_frame : 1;
            bool cheats_enabled : 1;
            bool cursor_enabled : 1;
            bool dev_enabled : 1;
            bool dev_liveUpdateLight : 1;
            bool dev_liveFollowLight : 1;
        };
    };
    bool freecam, godmode, noclip, save_on_exit, post_process_enabled;
    void Init();
    bool LoadRegistries(char* textures=nullptr, char* tiles=nullptr, char* entities=nullptr, char* scripts=nullptr);
    void LoadConfigs();
    void LoadData();
    char* LoadIndex();
    void OpenWindow(char* title);
    void InitMesher();
    void InitCamera();
    void InitImGui();
    bool LoadLevel(char* name=nullptr);
    void UnloadLevel();
    bool TryLoadLevel(char* name);
    void BeforeMainLoop();
    void Draw();
    void HandleInputs(float dt);
    void Update(float dt);
    void EndWindow();
    void SaveConfigs();
    void TakeScreenshot(Texture2D texture);
    void ResizeWindow();
};