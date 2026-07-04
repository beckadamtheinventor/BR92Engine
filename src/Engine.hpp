#pragma once

#include "Configs.hpp"
#include "Entity.hpp"
#include "imgui.h"
#include "raylib.h"
#ifdef VR_SUPPORT
#include "openxr/openxr.h"
#include "rlOpenXR.h"
#endif

#define PLAYER_SPEED 1.5f

extern EntityRenderer* GlobalEntityRenderer;

class BR92Engine {
    public:
#ifdef VR_SUPPORT
    typedef struct
    {
        XrActionSet actionset;

        XrAction hand_pose_action;
        XrPath hand_sub_paths[2];
        XrSpace hand_spaces[2];

        XrAction hand_activate_action;
    } XRInputBindings;
#endif

    MainConfig cfg;
    ShaderConfig scfg;
    DevConfig dcfg;
    char* levelFileName=nullptr;
    Shader postShader;
    RenderTexture2D gameTexture;
    RenderTexture2D screenTexture;
    RenderTexture2D menuTexture;
    Camera3D camera;
    ImVec2 gameWindowPosition;
    Vector3 dev_lightPosition;
    float deltatime;
    float dev_lightValue;
    float dev_lightColor[3];
    float mouseSensitivity, playerSpeed, playerMomentumVertical;
    float first_frame_timer;
    int renderScale, targetFps;
    unsigned int postVao;
#ifdef VR_SUPPORT
    struct xr {
        XRInputBindings bindings;
        RLHand leftHand, rightHand;
        Model handModel;
    } xr;
#endif
    union {
        int _flags;
        struct {
            bool drawing_menus : 1;
            bool cheats_enabled : 1;
            bool cursor_enabled : 1;
            bool dev_enabled : 1;
            bool vr_mode : 1;
        };
    };
    bool dev_liveUpdateLight, dev_liveFollowLight;
    bool freecam, godmode, noclip, save_on_exit, ascii_shader_enabled;
    short keyForward, keyBackward, keyLeft, keyRight, keyJump, keySprint, keyCrouch, keyUp, keyDown;
    void Init();
    bool LoadRegistries(char* textures=nullptr, char* tiles=nullptr, char* entities=nullptr, char* scripts=nullptr);
    void LoadConfigs();
    void LoadData();
    char* LoadIndex();
    bool OpenWindowVR(char* title);
    void OpenWindow(char* title);
    void InitMesher();
    void InitCamera();
    void InitImGui();
#ifdef VR_SUPPORT
    void SetupXRInputBindings();
    void AssignXRHandInputBindings();
    float GetXRActionValueFloat(XrAction action, XrPath sub_path);
#endif
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

extern BR92Engine* GlobalEngine;
