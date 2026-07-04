
#include "EntityRegistry.hpp"
#include "TileRegistry.hpp"
#include "imgui.h"
#include "imguiThemes.h"
#include "raylib.h"
#include "rlImGui.h"
#include "raymath.h"
#include "rcamera.h"

#include "Engine.hpp"
#include "AssetPath.hpp"
#include "Helpers.hpp"
#include "Registries.hpp"
#include "MapData.hpp"
#include "ScriptEngine/ScriptInterface.hpp"
#include "ShaderLoader.hpp"
#include <ctime>

const char* MAIN_CONFIG_FILE = "config.dat";
const char* SHADER_CONFIG_FILE = "assets/shaders/cfg.dat";
const char* DEV_CONFIG_FILE = "dev.dat";
const char* VERSION_STRING = "0.0.2-indev";

EntityRenderer* GlobalEntityRenderer=nullptr;
BR92Engine* GlobalEngine=nullptr;

#pragma region Init
void BR92Engine::Init() {
	TraceLog(LOG_INFO, "Initializing Registries...");
    GlobalMapTileRegistry = new MapTileRegistry();
    GlobalTextureRegistry = new TextureRegistry();
	GlobalEntityRegistry = new EntityRegistry();
	GlobalScriptRegistry = new ScriptRegistry();
	GloablScriptInterface = new ScriptInterface();
	GlobalEntityRenderer = new EntityRenderer();
	GlobalEngine = this;
}

#pragma endregion

#pragma region LoadRegistries
bool BR92Engine::LoadRegistries(char* textures, char* tiles, char* entities, char* scripts) {
	TraceLog(LOG_INFO, "Loading Registries...");
    if (textures == nullptr) {
        textures = AssetPath::root("textures", "json");
    }
	TraceLog(LOG_INFO, "Loading Textures...");
	if (!GlobalTextureRegistry->load(textures)) {
        return false;
    }
    if (tiles == nullptr) {
        tiles = AssetPath::root("tiles", "json");
    }
	TraceLog(LOG_INFO, "Loading Tiles...");
    if (!GlobalMapTileRegistry->load(tiles, GlobalTextureRegistry)) {
        return false;
    }
	if (scripts == nullptr) {
		scripts = AssetPath::root("scripts", "json");
	}
	TraceLog(LOG_INFO, "Loading Scripts...");
	if (!GlobalScriptRegistry->load(scripts, GloablScriptInterface)) {
		return false;
	}
	if (entities == nullptr) {
		entities = AssetPath::root("entities", "json");
	}
	TraceLog(LOG_INFO, "Loading Entities...");
	if (!GlobalEntityRegistry->load(entities, GlobalTextureRegistry)) {
		return false;
	}
    return true;
}
#pragma endregion

#pragma region LoadConfigs
void BR92Engine::LoadConfigs() {
	TraceLog(LOG_INFO, "Loading Configs...");
	// Initialize main config and set defaults
	cfg = MainConfig(MAIN_CONFIG_FILE);
	// Initialize shader config and set defaults
	scfg = ShaderConfig(SHADER_CONFIG_FILE);
	// Initialize dev config and set defaults
	dcfg = DevConfig(DEV_CONFIG_FILE);
}
#pragma endregion

#pragma region LoadData
void BR92Engine::LoadData() {
	TraceLog(LOG_INFO, "Loading Map Data...");
    if (GlobalMapData == nullptr) {
        GlobalMapData = new MapData();
    }
	GlobalMapData->SetTextureRegistry(GlobalTextureRegistry);
	GlobalMapData->SetTileRegistry(GlobalMapTileRegistry);
	GlobalMapData->renderDistance = cfg["RenderDistance"].get<float>();
	GlobalMapData->fogColor[0] = scfg["FogColorR"].get<float>() * 1/255.0f;
	GlobalMapData->fogColor[1] = scfg["FogColorG"].get<float>() * 1/255.0f;
	GlobalMapData->fogColor[2] = scfg["FogColorB"].get<float>() * 1/255.0f;
	GlobalMapData->fogColor[3] = scfg["FogColorA"].get<float>() * 1/255.0f;
	GlobalMapData->fogMin = scfg["FogMin"].get<float>();
	GlobalMapData->fogMax = scfg["FogMax"].get<float>();
	GlobalMapData->lightLevel = scfg["LightLevel"].get<float>();
}
#pragma endregion

#pragma region LoadLevel
bool BR92Engine::LoadLevel(char* name) {
    if (name == nullptr) {
        levelFileName = LoadIndex();
    } else {
        levelFileName = name;
    }
	TraceLog(LOG_INFO, "Loading Level %s...", name);
	RBuffer readbuf;
	readbuf.open(levelFileName);
	GlobalEntityRenderer->clear();
	if (readbuf.available() > 0) {
		if (!GlobalMapData->LoadMap(readbuf)) {
			AssetFormatError(levelFileName);
            return false;
		}
	} else {
		MissingAssetError(levelFileName);
        return false;
	}
	// if (!map->HasLoadedLightmaps()) {
	// 	map->BuildLighting();
	// }
	GlobalEntityRenderer->Init();
	GlobalMapData->GenerateMesh();
	GlobalMapData->UploadMap();
    Vector3 delta = Vector3Subtract(camera.target, camera.position);
    camera.position = {0, PLAYER_HEIGHT, 0};
    camera.target = {delta.x, delta.y+PLAYER_HEIGHT, delta.z};
    return true;
}
#pragma endregion

#pragma region UnloadLevel
void BR92Engine::UnloadLevel() {
	TraceLog(LOG_INFO, "Unloading Level...");
    GlobalMapData->ClearMap();
}
#pragma endregion

#pragma region LoadIndex
char* BR92Engine::LoadIndex() {
	std::ifstream fd(AssetPath::root("index", nullptr));
	if (fd.is_open()) {
		size_t count = fstreamlen(fd);
		char* data = new char[count+1];
		fd.read(data, count);
		data[count] = 0;
		fd.close();
		char* name = AssetPath::clone(AssetPath::level(data));
		delete[] data;
        return name;
	} else {
		MissingAssetError(AssetPath::root("index", nullptr));
	}
    return nullptr;
}
#pragma endregion

#pragma region OpenWindow
#ifdef VR_SUPPORT
bool BR92Engine::OpenWindowVR(char* title) {
	OpenWindow(title);
	if (!rlOpenXRSetup()) {
		TraceLog(LOG_INFO, "[--!--] Failed to initialize rlOpenXR! Starting in Desktop mode... [--!--]");
		return false;
	}
	rlSetFramebufferWidth(rlGetFramebufferWidth()*2);

	SetupXRInputBindings();
	xr.leftHand.handedness = RLOPENXR_HAND_LEFT;
	xr.rightHand.handedness = RLOPENXR_HAND_RIGHT;
	AssignXRHandInputBindings();

	xr.handModel = LoadModelFromMesh(GenMeshCube(0.1f, 0.1f, 0.1f));

	SetTargetFPS((targetFps = -1));
	vr_mode = true;
	return true;
}
#endif
void BR92Engine::OpenWindow(char* title) {
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(320, 240, title);

	try {
		size_t win_w = cfg["WindowSizeX"].get<unsigned int>();
		size_t win_h = cfg["WindowSizeY"].get<unsigned int>();
		int win_x = cfg["WindowPosX"].get<int>();
		int win_y = cfg["WindowPosY"].get<int>();
		int m = GetCurrentMonitor();
		if (win_w > GetMonitorWidth(m)) {
			win_w = GetMonitorWidth(m);
			win_x = 0;
		}
		if (win_h > GetMonitorHeight(m)) {
			win_h = GetMonitorHeight(m) - 40;
			win_y = 20;
		}
		SetWindowSize(win_w, win_h);

		if (cfg["WindowFullscreen"].get<bool>()) {
			ToggleFullscreen();
		} else if (cfg["WindowMaximized"].get<bool>()) {
			MaximizeWindow();
		} else {
			SetWindowPosition(win_x, win_y);
		}
		targetFps = cfg["TargetFPS"].get<float>();
		if (targetFps != -1 && targetFps < 15) {
			targetFps = 15;
		}
		mouseSensitivity = cfg["MouseSensitivity"].get<float>();
	} catch (JSON::exception err) {
		TraceLog(LOG_ERROR, "Failed to load main config!");
	}
	SetWindowMinSize(320, 240);
	SetTargetFPS(targetFps);
	SetExitKey(-1);
	GlobalEntityRenderer->PreInit();
	postShader = ShaderLoader::load(AssetPath::shader("post"));
	glGenVertexArrays(1, &postVao);

	SetMousePosition(300, 220);
	first_frame_timer = 0.125f;
	vr_mode = false;
}
#pragma endregion

#pragma region InitMesher
void BR92Engine::InitMesher() {
	TraceLog(LOG_INFO, "Initializing Mesher...");
	float aspect = GetRenderHeight() / (float)GetRenderWidth();
	renderScale = cfg["RenderScale"].get<unsigned int>();
	gameTexture = LoadRenderTexture(renderScale, renderScale*aspect);
	screenTexture = LoadRenderTexture(GetRenderWidth(), GetRenderHeight());
	if (vr_mode) {
		menuTexture = LoadRenderTexture(640, 640);
	}

	GlobalMapData->BuildAtlas();
	GlobalMapData->InitMesher(gameTexture.depth.id);
}
#pragma endregion

#pragma region InitCamera
void BR92Engine::InitCamera() {
	TraceLog(LOG_INFO, "Initializing Main Camera...");
	camera.fovy = cfg["FOVY"].get<float>();
	camera.position.x = cfg["PlayerX"].get<float>();
	camera.position.y = cfg["PlayerY"].get<float>();
	camera.position.z = cfg["PlayerZ"].get<float>();;
	camera.target.x = cfg["PlayerTX"].get<float>();
	camera.target.y = cfg["PlayerTY"].get<float>();
	camera.target.z = cfg["PlayerTZ"].get<float>();
	camera.up.x = cfg["PlayerUX"].get<float>();
	camera.up.y = cfg["PlayerUY"].get<float>();;
	camera.up.z = cfg["PlayerUZ"].get<float>();;
	camera.projection = CAMERA_PERSPECTIVE;
}
#pragma endregion

#pragma region InitImGui
void BR92Engine::InitImGui() {
	TraceLog(LOG_INFO, "Initializing ImGui...");
	rlImGuiSetup(true);

	//you can use whatever imgui theme you like!
	ImGui::StyleColorsDark();
	//imguiThemes::yellow();
	//imguiThemes::gray();
	//imguiThemes::green();
	//imguiThemes::red();
	imguiThemes::embraceTheDarkness();

	ImGuiIO &io_imgui = ImGui::GetIO();
	io_imgui.FontGlobalScale = 1.8f;
	io_imgui.ConfigWindowsMoveFromTitleBarOnly = true;

	ImGuiStyle &style = ImGui::GetStyle();
	if (io_imgui.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.1f;
		style.Colors[ImGuiCol_WindowBg].w = 0.5f;
		//style.Colors[ImGuiCol_DockingEmptyBg].w = 0.f;
	}
	io_imgui.WantCaptureKeyboard = false;
	io_imgui.WantCaptureMouse = false;
}
#pragma endregion

#pragma region BeforeMainLoop
void BR92Engine::BeforeMainLoop() {

	save_on_exit = dcfg["SaveMapOnExit"].get<bool>();
	cheats_enabled = cfg["CheatsEnabled"].get<bool>();
	dev_enabled = dcfg["DevEnabled"].get<bool>();
	drawing_menus = false;
	cursor_enabled = false;
	DisableCursor();
	first_frame_timer = true;
	freecam = false;
	godmode = false;
	noclip = false;
	ascii_shader_enabled = false;
	if (cheats_enabled) {
		freecam = cfg["FreecamEnabled"].get<bool>();
		godmode = cfg["GodmodeEnabled"].get<bool>();
		noclip = cfg["NoclipEnabled"].get<bool>();
	}
	playerSpeed = PLAYER_SPEED;
	playerMomentumVertical = 0;
	dev_lightValue = 0;
	dev_lightColor[0] = dev_lightColor[1] = dev_lightColor[2] = 1.0f;
	dev_liveUpdateLight = false;
	dev_liveFollowLight = false;
}
#pragma endregion

#pragma region TryLoadLevel
bool BR92Engine::TryLoadLevel(char* name) {
	TraceLog(LOG_INFO, "Attempting to load level %s...", name);
    char* oldname = levelFileName;
    UnloadLevel();
    name = AssetPath::clone(name);
    if (!LoadLevel(name)) {
        if (!LoadLevel(AssetPath::level(name))) {
            // if (!LoadLevel(oldname)) {
            //     exit(1);
            // }
			TraceLog(LOG_INFO, "Failed to load level %s, reloading original level...");
            LoadLevel(oldname);
            delete [] name;
            return false;
        }
    }
    delete [] name;
    return true;
}

#pragma endregion

#pragma region Draw
void BR92Engine::Draw() {
		if (IsWindowResized()) {
			ResizeWindow();
		}

#pragma region VR Mode Drawing
#ifdef VR_SUPPORT
		if (vr_mode) {
			SetTargetFPS(-1);
			rlOpenXRUpdate();
			rlOpenXRSyncSingleActionSet(xr.bindings.actionset);
			rlOpenXRUpdateHands(&xr.leftHand, &xr.rightHand);
			UpdateCamera(&camera, CAMERA_FREE); // Use mouse control as a debug option when no HMD is available
			rlOpenXRUpdateCamera(&camera); // If the HMD is available, set the camera position to the HMD position

			Color clearcolor = {
				(unsigned char)(GlobalMapData->fogColor[0]*255.0f),
				(unsigned char)(GlobalMapData->fogColor[1]*255.0f),
				(unsigned char)(GlobalMapData->fogColor[2]*255.0f),
				(unsigned char)(GlobalMapData->fogColor[3]*255.0f)
			};
			// rlOpenXRBegin() returns false when OpenXR reports to skip the frame (The HMD is inactive).
			// Optionally rlOpenXRBeginMockHMD() can be chained to always render. It will render into a "Mock" backbuffer.
			if (rlOpenXRBegin() || rlOpenXRBeginMockHMD()) { // Render to OpenXR backbuffer
				ClearBackground(clearcolor);

				BeginMode3D(camera);
				// Draw Hands
				Vector3 left_hand_axis;
				float left_hand_angle;
				QuaternionToAxisAngle(xr.leftHand.orientation, &left_hand_axis, &left_hand_angle);

				Vector3 right_hand_axis;
				float right_hand_angle;
				QuaternionToAxisAngle(xr.rightHand.orientation, &right_hand_axis, &right_hand_angle);

				float left_value = GetXRActionValueFloat(xr.bindings.hand_activate_action, xr.bindings.hand_sub_paths[RLOPENXR_HAND_LEFT]);
				float right_value = GetXRActionValueFloat(xr.bindings.hand_activate_action, xr.bindings.hand_sub_paths[RLOPENXR_HAND_RIGHT]);
				Color left_color = left_value > 0.75f ? GREEN : ORANGE;
				Color right_color = right_value > 0.75f ? GREEN : YELLOW;

				DrawModelEx(xr.handModel, xr.leftHand.position, left_hand_axis, left_hand_angle * RAD2DEG, Vector3One(), left_color);
				DrawModelEx(xr.handModel, xr.rightHand.position, right_hand_axis, right_hand_angle * RAD2DEG, Vector3One(), right_color);

				GlobalMapData->Draw(camera.position, nullptr, renderScale, vr_mode);
				GlobalEntityRenderer->Draw(GlobalMapData, camera.position, renderScale, vr_mode);

				EndMode3D();
				rlOpenXRBlitToWindow(RLOPENXR_EYE_RIGHT, true);
				// rlOpenXRBlitToWindow(RLOPENXR_EYE_BOTH, false);
			}
			rlOpenXREnd();
#pragma endregion
		} else {
#endif
#pragma region Desktop Drawing
			BeginDrawing();
			// ClearBackground(BLACK);

			// render the scene
			BeginTextureMode(gameTexture);
			{
				Color tmp = {
					(unsigned char)(GlobalMapData->fogColor[0]*255.0f),
					(unsigned char)(GlobalMapData->fogColor[1]*255.0f),
					(unsigned char)(GlobalMapData->fogColor[2]*255.0f),
					(unsigned char)(GlobalMapData->fogColor[3]*255.0f)
				};
				ClearBackground(tmp);
			}
			BeginMode3D(camera);

			GlobalMapData->Draw(camera.position, nullptr, renderScale, vr_mode);
			// DrawPlane({0,0,0}, {5,5}, GRAY);
			GlobalEntityRenderer->Draw(GlobalMapData, camera.position, renderScale, vr_mode);

			EndMode3D();

			EndTextureMode();

			// render the HUD and effects
			BeginTextureMode(screenTexture);

			DrawTexturePro(gameTexture.texture,
				{0, (float)-gameTexture.texture.height, (float)gameTexture.texture.width, (float)-gameTexture.texture.height},
				{0, 0, (float)GetRenderWidth(), (float)GetRenderHeight()},
				{0,0}, 0.0f, WHITE);


			EndTextureMode();

			if (ascii_shader_enabled && IsShaderReady(postShader)) {
				glUseProgram(postShader.id);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, screenTexture.texture.id);
				unsigned int loc = GetShaderLocation(postShader, "screenTexture");
				glUniform1i(loc, 0);
				loc = GetShaderLocation(postShader, "resolution");
				glUniform2f(loc, screenTexture.texture.width, screenTexture.texture.height);
				glDisable(GL_DEPTH_TEST);
				glBindVertexArray(postVao);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				glBindVertexArray(0);
				glUseProgram(0);
			} else {
				DrawTexturePro(screenTexture.texture,
					{0, (float)-screenTexture.texture.height, (float)screenTexture.texture.width, (float)-screenTexture.texture.height},
					{0, 0, (float)GetRenderWidth(), (float)GetRenderHeight()},
					{0,0}, 0.0f, WHITE);
			}

			char buffer[64];
			DrawRectangle(1, 0, GetRenderWidth()-2, 23, DARKGRAY);
			DrawLine(1, 24, GetRenderWidth()-2, 24, BLACK);
			if (cheats_enabled) {
				snprintf(buffer, sizeof(buffer), "%d", (int)camera.position.x);
				DrawText(buffer, 100, 3, 20, WHITE);
				snprintf(buffer, sizeof(buffer), "%d", (int)camera.position.y);
				DrawText(buffer, 150, 3, 20, WHITE);
				snprintf(buffer, sizeof(buffer), "%d", (int)camera.position.z);
				DrawText(buffer, 200, 3, 20, WHITE);
				// snprintf(buffer, sizeof(buffer), "%f", GlobalEntityRenderer->get(0)->timer);
				// DrawText(buffer, 250, 3, 20, WHITE);
			}
			DrawFPS(4, 4);
#ifdef VR_SUPPORT
		}

		if (vr_mode) {
			BeginDrawing();
            DrawFPS(10, 10);
		}
#endif

		if (drawing_menus) {
			if (vr_mode) {
				BeginTextureMode(menuTexture);
			}
			rlImGuiBegin();

			// ImGui::PushStyleColor(ImGuiCol_WindowBg, {});
			// ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, {});
			// ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
			// ImGui::PopStyleColor(2);
#pragma endregion

// #pragma region UI: GameWindow
	// 		/* Game Window */
	// 		{
	// 			ImGui::Begin("Game View", nullptr, ImGuiViewportFlags_NoRendererClear|ImGuiWindowFlags_NoCollapse);
	// 			gameWindowPosition = ImGui::GetWindowPos();
	// 			if (first_frame_timer) {
	// 				ImGui::SetWindowSize({1920*0.75f, 1080*0.75f});
	// 				ImGui::SetWindowPos({0,0});
	// 			} else {
	// 				ImVec2 wsize = ImGui::GetWindowSize();
	// 				if (wsize.x < 128) {
	// 					wsize.x = 128;
	// 				}
	// 				if (wsize.y < 128) {
	// 					wsize.y = 128;
	// 				}
	// 				ImGui::SetWindowSize(wsize);
	// 				gameWindowPosition.x += wsize.x/2;
	// 				gameWindowPosition.y += wsize.y/2;
	// 			}
	// 			rlImGuiImageRenderTextureFit(&gameTexture, true);
	// 			ImGui::End();
	// 		}
// #pragma endregion

#pragma region UI: Config
			/* Configuration window */
			{
				ImGui::Begin("Config");

				bool windowIsFullscreen = IsWindowFullscreen();
				if (!vr_mode) {
					if (ImGui::BeginCombo("Target FPS", targetFps < 0 ? "Unlimited" : TextFormat("%u", targetFps))) {
						const int MIN_FPS = 15;
						float w = ImGui::CalcItemWidth();
						if (ImGui::Button("60", {w, 0})) {
							SetTargetFPS((targetFps = 60));
						}
						if (ImGui::Button("75", {w, 0})) {
							SetTargetFPS((targetFps = 75));
						}
						if (ImGui::Button("120", {w, 0})) {
							SetTargetFPS((targetFps = 120));
						}
						if (ImGui::Button("144", {w, 0})) {
							SetTargetFPS((targetFps = 144));
						}
						if (ImGui::Button("Unlimited", {w, 0})) {
							SetTargetFPS((targetFps = -1));
						}
						ImGui::PushID("Custom");
						if (ImGui::SliderInt(" ", &targetFps, MIN_FPS, 144)) {
							if (targetFps >= MIN_FPS) {
								SetTargetFPS(targetFps);
							} else {
								SetTargetFPS(MIN_FPS);
							}
						}
						ImGui::PopID();
						ImGui::EndCombo();
					}
					if (ImGui::Checkbox("Fullscreen", &windowIsFullscreen)) {
						ToggleFullscreen();
					}
					if (ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity, 0.05f, 1.0f)) {}
				}
				if (ImGui::SliderFloat("Render Distance", &GlobalMapData->renderDistance, 10.0f, 200.0f)) {}
				if (!vr_mode) {
					if (ImGui::BeginCombo("Render Scale", TextFormat("%u", renderScale))) {
						float w = ImGui::CalcItemWidth();
						if (ImGui::Button("80", {w, 0})) {
							renderScale = 80;
							ResizeWindow();
						}
						if (ImGui::Button("160", {w, 0})) {
							renderScale = 160;
							ResizeWindow();
						}
						if (ImGui::Button("320", {w, 0})) {
							renderScale = 320;
							ResizeWindow();
						}
						if (ImGui::Button("640", {w, 0})) {
							renderScale = 640;
							ResizeWindow();
						}
						if (ImGui::Button("1024", {w, 0})) {
							renderScale = 1024;
							ResizeWindow();
						}
						if (ImGui::Button("1920", {w, 0})) {
							renderScale = 1920;
							ResizeWindow();
						}
						if (ImGui::Button("3840", {w, 0})) {
							renderScale = 3840;
							ResizeWindow();
						}
						if (ImGui::Button("7680", {w, 0})) {
							renderScale = 7680;
							ResizeWindow();
						}
						ImGui::PushID("Custom");
						if (ImGui::SliderInt(" ", &renderScale, 80, 8192)) {
							ResizeWindow();
						}
						ImGui::PopID();
						ImGui::EndCombo();
					}
					ImGui::Checkbox("Enable Ascii shader", &ascii_shader_enabled);
				}
				if (ImGui::Button("Take Screenshot (F2)")) {
					this->TakeScreenshot(screenTexture.texture);
				}
				if (!vr_mode) {
					if (gameTexture.texture.width > 8192 || gameTexture.texture.height > 8192) {
						ImGui::Text("Full-res screen screenshot unavailable (>8192px)");
					} else if (ImGui::Button("Take Full-res Screenshot")) {
						this->TakeScreenshot(gameTexture.texture);
					}
				}
				ImGui::End();
			}
#pragma endregion

#pragma region UI: Shader CFG
			/* Shader Config Menu */
			if (dev_enabled) {
				ImGui::Begin("Shader Config");
				if (ImGui::ColorEdit4("Fog Color", GlobalMapData->fogColor)) {}
				if (ImGui::SliderFloat("Fog Min", &GlobalMapData->fogMin, 0.01f, 100.0f)) {}
				if (ImGui::SliderFloat("Fog Max", &GlobalMapData->fogMax, 0.01f, 100.0f)) {}
				if (ImGui::SliderFloat("Light", &GlobalMapData->lightLevel, 0.01f, 2.0f)) {}
				ImGui::End();
			}
#pragma endregion

#pragma region UI: Cheats
			/* Cheats Menu */
			if (cheats_enabled) {
                static char tempLevelName[256] = {0};
				ImGui::Begin("Cheats");
				if (ImGui::Checkbox("Freecam", &freecam)) {
					playerMomentumVertical = 0;
				}
				if (ImGui::Checkbox("Noclip", &noclip)) {
					playerMomentumVertical = 0;
				}
				if (ImGui::Checkbox("Godmode", &godmode)) {}
				if (ImGui::InputFloat("Speed", &playerSpeed)) {}
                ImGui::InputText("Path", tempLevelName, sizeof(tempLevelName));
                if (ImGui::Button("Load Level")) {
                    TryLoadLevel(tempLevelName);
                }
				ImGui::End();
			}
#pragma endregion

#pragma region UI: Dev Menu
			/* Dev Menu */
			if (0) { // disabled for now
				static bool fillArea = false;
				static bool position1Set = false;
				static bool position2Set = false;
				static bool lightChanged = false;
				static Vector3 fillPosition1 = {0,0,0};
				static Vector3 fillPosition2 = {0,0,0};
				ImGui::Begin("Dev Menu");
				if (ImGui::Button("Save Map")) {
					GlobalMapData->SaveMap(levelFileName);
				}
				ImGui::Checkbox("Save on Exit", &save_on_exit);
				lightChanged |= ImGui::SliderFloat("Light value", &dev_lightValue, 0, 255);
				lightChanged |= ImGui::ColorEdit3("Light Color", dev_lightColor);
				ImGui::Checkbox("Live Update", &dev_liveUpdateLight);
				if (dev_liveUpdateLight || ImGui::Button("Set Light")) {
					GlobalMapData->setLight(dev_lightPosition, dev_lightValue, dev_lightColor[2]*255.0f, dev_lightColor[1]*255.0f, dev_lightColor[0]*255.0f);
					lightChanged = false;
				}
				ImGui::Checkbox("Live Follow", &dev_liveFollowLight);
				if (dev_liveFollowLight || ImGui::Button("Set Position")) {
					dev_lightPosition = camera.position;
				}
				fillArea |= ImGui::Button("Fill Area");
				if (fillArea) {
					if (!position1Set) {
						position1Set |= ImGui::Button("Position 1");
						if (position1Set) {
							fillPosition1 = camera.position;
						}
					} else if (!position2Set) {
						position2Set |= ImGui::Button("Position 2");
						if (position2Set) {
							fillPosition2 = camera.position;
							GlobalMapData->setLight(fillPosition1, fillPosition2, dev_lightValue, dev_lightColor[2]*255.0f, dev_lightColor[1]*255.0f, dev_lightColor[0]*255.0f);
							position2Set = position1Set = fillArea = false;
						}
					}
				}
				ImGui::End();
			}
#pragma endregion


#pragma region End Drawing
			rlImGuiEnd();

			if (!vr_mode) {
				ImGuiIO& io_imgui = ImGui::GetIO();
				if (io_imgui.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
				{
					ImGui::UpdatePlatformWindows();
					ImGui::RenderPlatformWindowsDefault();
				}
			}

			if (vr_mode) {
				EndTextureMode();
			}
		}
		
		EndDrawing();
#pragma endregion
}
#pragma endregion

#pragma region HandleInputs
void BR92Engine::HandleInputs(float dt) {
    if (IsFileDropped()) {
        FilePathList list = LoadDroppedFiles();
        if (list.count > 0) {
            TryLoadLevel(list.paths[0]);
        }
		UnloadDroppedFiles(list);
    }
	if (first_frame_timer > 0) {
		first_frame_timer -= dt;
	}
	if (!vr_mode) {
		if (!cursor_enabled && first_frame_timer <= 0) {
			Vector2 mouseDelta = GetMouseDelta();
			SetMousePosition(gameWindowPosition.x, gameWindowPosition.y);
			float delta = playerSpeed*dt;
			if (IsKeyDown(KEY_LEFT_CONTROL)) {
				delta *= 1.5f;
			}
			Vector3 oldPosition = camera.position;
			if (IsKeyDown(KEY_W)) {
				CameraMoveForward(&camera, delta, !freecam);
			}
			if (IsKeyDown(KEY_S)) {
				CameraMoveForward(&camera, -delta, !freecam);
			}
			if (IsKeyDown(KEY_A)) {
				CameraMoveRight(&camera, -delta, !freecam);
			}
			if (IsKeyDown(KEY_D)) {
				CameraMoveRight(&camera, delta, !freecam);
			}
			// if (IsKeyDown(KEY_SPACE)) {
			// 	unsigned short tid = GlobalMapData->get(oldPosition);
			// 	if (tid != 0) {
			// 		MapTile* tile = GlobalMapTileRegistry->of(tid);
			// 		if (tile != nullptr) {
			// 			if (tile->solidFloor) {
			// 				playerMomentumVertical += PLAYER_JUMP;
			// 			}
			// 		}
			// 	}
			// }
			Vector3 movement = Vector3Subtract(camera.position, oldPosition);
			if (freecam) {
				if (IsKeyDown(KEY_Z)) {
					CameraMoveUp(&camera, delta);
				}
				if (IsKeyDown(KEY_X)) {
					CameraMoveUp(&camera, -delta);
				}
			} else {
				Vector3 adjustedPosition = GlobalMapData->MoveTo(oldPosition, movement, noclip);
				if (!noclip) {
					adjustedPosition = GlobalMapData->ApplyGravity(adjustedPosition, playerMomentumVertical, dt);
				}
				Vector3 delta = Vector3Subtract(adjustedPosition, camera.position);
				camera.position = Vector3Add(camera.position, delta);
				camera.target = Vector3Add(camera.target, delta);
			}
			CameraYaw(&camera, -mouseDelta.x*dt*mouseSensitivity, false);
			CameraPitch(&camera, -mouseDelta.y*dt*mouseSensitivity, true, false, false);
			if (IsKeyPressed(KEY_ZERO)) {
				cheats_enabled = !cheats_enabled;
			}
			if (IsKeyPressed(KEY_GRAVE)) {
				dev_enabled = !dev_enabled;
			}
			if (IsKeyPressed(KEY_F2)) {
				this->TakeScreenshot(screenTexture.texture);
			}
		}

		if (IsKeyPressed(KEY_ESCAPE)) {
			if (drawing_menus) {
				drawing_menus = false;
				cursor_enabled = false;
				DisableCursor();
			} else {
				drawing_menus = true;
				cursor_enabled = true;
				EnableCursor();
			}
		}
	}
}
#pragma endregion

#pragma region Update
void BR92Engine::Update(float dt) {
	deltatime = dt;
	if (!drawing_menus) {
		GlobalEntityRenderer->Update(GlobalMapData, camera.position, dt);
	}
}
#pragma endregion

#pragma region EndWindow
void BR92Engine::EndWindow() {
	if (save_on_exit) {
		GlobalMapData->SaveMap(levelFileName);
	}

	rlImGuiShutdown();
#ifdef VR_SUPPORT
	if (vr_mode) {
		rlOpenXRShutdown();
		UnloadModel(xr.handModel);
	}
#endif
	CloseWindow();
}
#pragma endregion

#pragma region SaveConfigs
void BR92Engine::SaveConfigs() {
	Vector2 windowPos = GetWindowPosition();
	cfg["TargetFPS"] = targetFps;
	cfg["WindowSizeX"] = GetRenderWidth();
	cfg["WindowSizeY"] = GetRenderHeight();
	cfg["WindowPosX"] = windowPos.x;
	cfg["WindowPosY"] = windowPos.y + 20;
	cfg["WindowMaximized"] = IsWindowMaximized();
	cfg["WindowFullscreen"] = IsWindowFullscreen();
    cfg["FOVY"] = camera.fovy;
    cfg["PlayerX"] = camera.position.x;
    cfg["PlayerY"] = camera.position.y;
    cfg["PlayerZ"] = camera.position.z;
    cfg["PlayerTX"] = camera.target.x;
    cfg["PlayerTY"] = camera.target.y;
    cfg["PlayerTZ"] = camera.target.z;
    cfg["PlayerUX"] = camera.up.x;
    cfg["PlayerUY"] = camera.up.y;
    cfg["PlayerUZ"] = camera.up.z;
	cfg["RenderDistance"] = GlobalMapData->renderDistance;
	cfg["MouseSensitivity"] = mouseSensitivity;
	cfg["CheatsEnabled"] = (bool)cheats_enabled;
	cfg["FreecamEnabled"] = freecam;
	cfg["GodmodeEnabled"] = godmode;
	cfg["NoclipEnabled"] = noclip;
	cfg["RenderScale"] = renderScale;
	cfg.save();

	scfg["FogColorR"] = (unsigned int)GlobalMapData->fogColor[0]*255.0f;
	scfg["FogColorG"] = (unsigned int)GlobalMapData->fogColor[1]*255.0f;
	scfg["FogColorB"] = (unsigned int)GlobalMapData->fogColor[2]*255.0f;
	scfg["FogColorA"] = (unsigned int)GlobalMapData->fogColor[3]*255.0f;
	scfg["FogMin"] = GlobalMapData->fogMin;
	scfg["FogMax"] = GlobalMapData->fogMax;
	scfg["LightLevel"] = GlobalMapData->lightLevel;
	scfg.save();
}
#pragma endregion

#pragma region TakeScreenshot

void BR92Engine::TakeScreenshot(Texture2D texture) {
	char buf[128];
	time_t t = time(nullptr);
	struct tm* ts = localtime(&t);
	strftime(buf, sizeof(buf), "BR92_screenshot_%Y_%A_%B_%d_%I_%M_%S_%p.png", ts);
	Image screenimg = LoadImageFromTexture(texture);
	ImageFlipVertical(&screenimg);
	ExportImage(screenimg, buf);
	UnloadImage(screenimg);
}

#pragma endregion

#pragma region ResizeWindow

void BR92Engine::ResizeWindow() {
	UnloadRenderTexture(gameTexture);
	UnloadRenderTexture(screenTexture);
	float aspect = GetRenderHeight() / (float)GetRenderWidth();
	gameTexture = LoadRenderTexture(renderScale, renderScale*aspect);
	screenTexture = LoadRenderTexture(GetRenderWidth(), GetRenderHeight());
}

#pragma endregion

#pragma region XR Binding
#ifdef VR_SUPPORT
void BR92Engine::SetupXRInputBindings() {
	const RLOpenXRData* xr = rlOpenXRData();

	XrResult result = xrStringToPath(xr->instance, "/user/hand/left", &this->xr.bindings.hand_sub_paths[RLOPENXR_HAND_LEFT]);
	assert(XR_SUCCEEDED(result) && "Could not convert Left hand string to path.");
	result = xrStringToPath(xr->instance, "/user/hand/right", &this->xr.bindings.hand_sub_paths[RLOPENXR_HAND_RIGHT]);
	assert(XR_SUCCEEDED(result) && "Could not convert Right hand string to path.");

	XrActionSetCreateInfo actionset_info;
	actionset_info.type = XR_TYPE_ACTION_SET_CREATE_INFO;
	actionset_info.next = NULL;
	strncpy_s(actionset_info.actionSetName, XR_MAX_ACTION_SET_NAME_SIZE, 
		"rlopenxr_hello_hands_actionset", XR_MAX_ACTION_SET_NAME_SIZE);
	strncpy_s(actionset_info.localizedActionSetName, XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE, 
		"OpenXR Hello Hands ActionSet", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE);
	actionset_info.priority = 0;

	result = xrCreateActionSet(xr->instance, &actionset_info, &this->xr.bindings.actionset);
	assert(XR_SUCCEEDED(result) && "Failed to create actionset.");

	{
		XrActionCreateInfo action_info;
		action_info.type = XR_TYPE_ACTION_CREATE_INFO;
		action_info.next = NULL;
		strncpy_s(action_info.actionName, XR_MAX_ACTION_NAME_SIZE, 
			"handpose", XR_MAX_ACTION_NAME_SIZE);
		action_info.actionType = XR_ACTION_TYPE_POSE_INPUT;
		action_info.countSubactionPaths = RLOPENXR_HAND_COUNT;
		action_info.subactionPaths = this->xr.bindings.hand_sub_paths;
		strncpy_s(action_info.localizedActionName, XR_MAX_LOCALIZED_ACTION_NAME_SIZE, 
			"Hand Pose", XR_MAX_LOCALIZED_ACTION_NAME_SIZE);

		result = xrCreateAction(this->xr.bindings.actionset, &action_info, &this->xr.bindings.hand_pose_action);
		assert(XR_SUCCEEDED(result) && "Failed to create hand pose action");
	}

	{
		XrActionCreateInfo action_info;
		action_info.type = XR_TYPE_ACTION_CREATE_INFO;
		action_info.next = NULL;
		strncpy_s(action_info.actionName, XR_MAX_ACTION_NAME_SIZE, 
			"activate", XR_MAX_ACTION_NAME_SIZE);
		action_info.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		action_info.countSubactionPaths = RLOPENXR_HAND_COUNT;
		action_info.subactionPaths = this->xr.bindings.hand_sub_paths;
		strncpy_s(action_info.localizedActionName, XR_MAX_LOCALIZED_ACTION_NAME_SIZE, 
			"Activate", XR_MAX_LOCALIZED_ACTION_NAME_SIZE);

		result = xrCreateAction(this->xr.bindings.actionset, &action_info, &this->xr.bindings.hand_activate_action);
		assert(XR_SUCCEEDED(result) && "Failed to create hand activate action");
	}

	// poses can't be queried directly, we need to create a space for each
	for (int hand = 0; hand < RLOPENXR_HAND_COUNT; hand++) {
		XrPosef identity_pose = { { 0, 0, 0, 1}, {0, 0, 0} };

		XrActionSpaceCreateInfo action_space_info;
		action_space_info.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
		action_space_info.next = NULL;
		action_space_info.action = this->xr.bindings.hand_pose_action;
		action_space_info.subactionPath = this->xr.bindings.hand_sub_paths[hand];
		action_space_info.poseInActionSpace = identity_pose;

		result = xrCreateActionSpace(xr->session, &action_space_info, &this->xr.bindings.hand_spaces[hand]);
		assert(XR_SUCCEEDED(result) && "failed to create hand %d pose space");
	}

	XrPath grip_pose_paths[2] = { 0 };
	xrStringToPath(xr->instance, "/user/hand/left/input/grip/pose", &grip_pose_paths[RLOPENXR_HAND_LEFT]);
	xrStringToPath(xr->instance, "/user/hand/right/input/grip/pose", &grip_pose_paths[RLOPENXR_HAND_RIGHT]);

	XrPath activate_paths[2] = { 0 };
	xrStringToPath(xr->instance, "/user/hand/left/input/trigger/value", &activate_paths[RLOPENXR_HAND_LEFT]);
	xrStringToPath(xr->instance, "/user/hand/right/input/trigger/value", &activate_paths[RLOPENXR_HAND_RIGHT]);

	// khr/simple_controller Interaction Profile
	{
		XrPath interaction_profile_path;
		result = xrStringToPath(xr->instance, "/interaction_profiles/khr/simple_controller", &interaction_profile_path);
		assert(XR_SUCCEEDED(result) && "failed to get interaction profile");

		XrActionSuggestedBinding action_suggested_bindings[] = {
			{ this->xr.bindings.hand_pose_action, grip_pose_paths[RLOPENXR_HAND_LEFT] },
			{ this->xr.bindings.hand_pose_action, grip_pose_paths[RLOPENXR_HAND_RIGHT] },
		};
		const int action_suggested_bindings_count = sizeof(action_suggested_bindings) / sizeof(action_suggested_bindings[0]);

		XrInteractionProfileSuggestedBinding suggested_bindings;
		suggested_bindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
		suggested_bindings.next = NULL;
		suggested_bindings.interactionProfile = interaction_profile_path;
		suggested_bindings.countSuggestedBindings = action_suggested_bindings_count;
		suggested_bindings.suggestedBindings = action_suggested_bindings;

		result = xrSuggestInteractionProfileBindings(xr->instance, &suggested_bindings);
		assert(XR_SUCCEEDED(result) && "failed to suggest bindings for khr/simple_controller");
	}

	// oculus/touch_controller Interaction Profile
	{
		XrPath interaction_profile_path;
		result = xrStringToPath(xr->instance, "/interaction_profiles/oculus/touch_controller", &interaction_profile_path);
		assert(XR_SUCCEEDED(result) && "failed to get interaction profile");

		XrActionSuggestedBinding action_suggested_bindings[] = {
			{ this->xr.bindings.hand_pose_action, grip_pose_paths[RLOPENXR_HAND_LEFT]},
			{ this->xr.bindings.hand_pose_action, grip_pose_paths[RLOPENXR_HAND_RIGHT]},
			{ this->xr.bindings.hand_activate_action, activate_paths[RLOPENXR_HAND_LEFT] },
			{ this->xr.bindings.hand_activate_action, activate_paths[RLOPENXR_HAND_RIGHT] },
		};
		const int action_suggested_bindings_count = sizeof(action_suggested_bindings) / sizeof(action_suggested_bindings[0]);

		XrInteractionProfileSuggestedBinding suggested_bindings;
		suggested_bindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
		suggested_bindings.next = NULL;
		suggested_bindings.interactionProfile = interaction_profile_path;
		suggested_bindings.countSuggestedBindings = action_suggested_bindings_count;
		suggested_bindings.suggestedBindings = action_suggested_bindings;

		result = xrSuggestInteractionProfileBindings(xr->instance, &suggested_bindings);
		assert(XR_SUCCEEDED(result) && "failed to suggest bindings for oculus/touch_controller");
	}

	XrSessionActionSetsAttachInfo actionset_attach_info;
	actionset_attach_info.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
	actionset_attach_info.next = NULL;
	actionset_attach_info.countActionSets = 1;
	actionset_attach_info.actionSets = &this->xr.bindings.actionset;
	result = xrAttachSessionActionSets(xr->session, &actionset_attach_info);
	assert(XR_SUCCEEDED(result) && "failed to attach action set");
}


void BR92Engine::AssignXRHandInputBindings()
{
	RLHand* hands[2] = { &xr.leftHand, &xr.rightHand };

	for (int i = 0; i < RLOPENXR_HAND_COUNT; ++i)
	{
		hands[i]->hand_pose_action = xr.bindings.hand_pose_action;
		hands[i]->hand_pose_subpath = xr.bindings.hand_sub_paths[i];
		hands[i]->hand_pose_space = xr.bindings.hand_spaces[i];
	}
}


float BR92Engine::GetXRActionValueFloat(XrAction action, XrPath sub_path) {
	XrActionStateGetInfo activate_state_get_info;
	activate_state_get_info.type = XR_TYPE_ACTION_STATE_GET_INFO;
	activate_state_get_info.next = NULL;
	activate_state_get_info.action = action;
	activate_state_get_info.subactionPath = sub_path;

	XrActionStateFloat activate_state;
	activate_state.type = XR_TYPE_ACTION_STATE_FLOAT;
	activate_state.next = NULL;
	XrResult result = xrGetActionStateFloat(rlOpenXRData()->session, &activate_state_get_info, &activate_state);
	assert(XR_SUCCEEDED(result) && "failed to get action state as a float");

	return activate_state.currentState;
}
#endif
#pragma endregion