#pragma once

#include "openvr.h"

#include "MapData.hpp"
#include "raymath.h"

class VRInterface {
    vr::IVRSystem* system;
    vr::EVRInitError err;
    uint32_t displayWidth, displayHeight;
    bool vrIsRunning;
    public:
    bool VRRunning() {
        return vrIsRunning;
    }
    bool VRPresent() {
        return vr::VR_IsHmdPresent();
    }
    bool Init(bool force=false) {
        if (!VRPresent() && !force) {
            return false;
        }
        system = vr::VR_Init(&err, vr::VRApplication_Scene);
        if (system == nullptr || err != vr::VRInitError_None) {
            return false;
        }
        system->GetRecommendedRenderTargetSize(&displayWidth, &displayHeight);

        vrIsRunning = true;
        
        return true;
    }
    bool Deinit() {
        if (!vrIsRunning) {
            return false;
        }
        vr::VR_Shutdown();
        return true;
    }
    void DrawEye(MapData& data, vr::EVREye eye, float near=0.01f, float far=1e5) {
        if (!vrIsRunning) return;
        vr::HmdMatrix44_t _projMatrix = system->GetProjectionMatrix(eye, near, far);
        Matrix projMatrix = *(Matrix*)&_projMatrix;
        Vector3 cameraPosition = Vector3Transform({0,0,0}, projMatrix);
        data.Draw(cameraPosition, &projMatrix);
    }
    void SubmitDrawing() {
        if (!vrIsRunning) return;
        
    }
};