
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

const char* MAIN_CONFIG_FILE = "config.dat";
const char* SHADER_CONFIG_FILE = "assets/shaders/cfg.dat";
const char* DEV_CONFIG_FILE = "dev.dat";
const char* VERSION_STRING = "0.0.2-indev";
EntityRenderer* GlobalEntityRenderer=nullptr;

#pragma region Init
void BR92Engine::Init() {
    GlobalMapTileRegistry = new MapTileRegistry();
    GlobalTextureRegistry = new TextureRegistry();
	GlobalEntityRegistry = new EntityRegistry();
	GlobalScriptRegistry = new ScriptRegistry();
	GloablScriptInterface = new ScriptInterface();
	GlobalEntityRenderer = new EntityRenderer();
}
#pragma endregion

#pragma region LoadRegistries
bool BR92Engine::LoadRegistries(char* textures, char* tiles, char* entities, char* scripts) {
    if (textures == nullptr) {
        textures = AssetPath::root("textures", "json");
    }
	if (!GlobalTextureRegistry->load(textures)) {
        return false;
    }
    if (tiles == nullptr) {
        tiles = AssetPath::root("tiles", "json");
    }
    if (!GlobalMapTileRegistry->load(tiles, GlobalTextureRegistry)) {
        return false;
    }
	if (scripts == nullptr) {
		scripts = AssetPath::root("scripts", "json");
	}
	if (!GlobalScriptRegistry->load(scripts, GloablScriptInterface)) {
		return false;
	}
	if (entities == nullptr) {
		entities = AssetPath::root("entities", "json");
	}
	if (!GlobalEntityRegistry->load(entities, GlobalTextureRegistry)) {
		return false;
	}
    return true;
}
#pragma endregion

#pragma region LoadConfigs
void BR92Engine::LoadConfigs() {
	// Initialize main config and set defaults
	cfg = new MainConfig(MAIN_CONFIG_FILE);
	// Initialize shader config and set defaults
	scfg = new ShaderConfig(SHADER_CONFIG_FILE);
	// Initialize dev config and set defaults
	dcfg = new DevConfig(DEV_CONFIG_FILE);
}
#pragma endregion

#pragma region LoadData
void BR92Engine::LoadData() {
    if (GlobalMapData == nullptr) {
        GlobalMapData = new MapData();
    }
	GlobalMapData->SetTextureRegistry(GlobalTextureRegistry);
	GlobalMapData->SetTileRegistry(GlobalMapTileRegistry);
	GlobalMapData->renderDistance = cfg->getFloat("RenderDistance");
	GlobalMapData->fogColor[0] = scfg->getByte("FogColorR") * 1/255.0f;
	GlobalMapData->fogColor[1] = scfg->getByte("FogColorG") * 1/255.0f;
	GlobalMapData->fogColor[2] = scfg->getByte("FogColorB") * 1/255.0f;
	GlobalMapData->fogColor[3] = scfg->getByte("FogColorA") * 1/255.0f;
	GlobalMapData->fogMin = scfg->getFloat("FogMin");
	GlobalMapData->fogMax = scfg->getFloat("FogMax");
	GlobalMapData->lightLevel = scfg->getFloat("LightLevel");
}
#pragma endregion

#pragma region LoadLevel
bool BR92Engine::LoadLevel(char* name) {
    if (name == nullptr) {
        levelFileName = LoadIndex();
    } else {
        levelFileName = name;
    }
	RBuffer readbuf;
	readbuf.open(levelFileName);
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
	GlobalMapData->GenerateMesh();
	GlobalMapData->UploadMap();
	GlobalEntityRenderer->clear();
    Vector3 delta = Vector3Subtract(camera.target, camera.position);
    camera.position = {0, PLAYER_HEIGHT, 0};
    camera.target = {delta.x, delta.y+PLAYER_HEIGHT, delta.z};
    return true;
}
#pragma endregion

#pragma region UnloadLevel
void BR92Engine::UnloadLevel() {
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
void BR92Engine::OpenWindow(char* title) {
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(320, 240, title);

	size_t win_w = cfg->getUnsigned("WindowSizeX");
	size_t win_h = cfg->getUnsigned("WindowSizeY");
	int win_x = cfg->getInteger("WindowPosX");
	int win_y = cfg->getInteger("WindowPosY");
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

	if (cfg->getBool("WindowFullscreen")) {
		ToggleFullscreen();
	} else if (cfg->getBool("WindowMaximized")) {
		MaximizeWindow();
	} else {
		SetWindowPosition(win_x, win_y);
	}
	targetFps = cfg->getInteger("TargetFPS");
	if (targetFps != -1 && targetFps < 15) {
		targetFps = 15;
	}
	mouseSensitivity = cfg->getFloat("MouseSensitivity");

	SetWindowMinSize(320, 240);
	SetTargetFPS(targetFps);
	SetExitKey(-1);
	GlobalEntityRenderer->init();
	postShader = ShaderLoader::load(AssetPath::shader("post"));
	glGenVertexArrays(1, &postVao);

	SetMousePosition(300, 220);
}
#pragma endregion

#pragma region InitMesher
void BR92Engine::InitMesher() {
	float aspect = GetRenderHeight() / (float)GetRenderWidth();
	renderScale = cfg->getUnsigned("RenderScale");
	gameTexture = LoadRenderTexture(renderScale, renderScale*aspect);
	screenTexture = LoadRenderTexture(GetRenderWidth(), GetRenderHeight());

	GlobalMapData->BuildAtlas();
	GlobalMapData->InitMesher(gameTexture.depth.id);
}
#pragma endregion

#pragma region InitCamera
void BR92Engine::InitCamera() {
	camera.fovy = cfg->getFloat("FOVY");
	camera.position.x = cfg->getFloat("PlayerX");
	camera.position.y = cfg->getFloat("PlayerY");
	camera.position.z = cfg->getFloat("PlayerZ");
	camera.target.x = cfg->getFloat("PlayerTX");
	camera.target.y = cfg->getFloat("PlayerTY");
	camera.target.z = cfg->getFloat("PlayerTZ");
	camera.up.x = cfg->getFloat("PlayerUX");
	camera.up.y = cfg->getFloat("PlayerUY");
	camera.up.z = cfg->getFloat("PlayerUZ");
	camera.projection = CAMERA_PERSPECTIVE;
}
#pragma endregion

#pragma region InitImGui
void BR92Engine::InitImGui() {
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

	save_on_exit = dcfg->getBool("SaveMapOnExit");
	cheats_enabled = cfg->getBool("CheatsEnabled");
	dev_enabled = dcfg->getBool("DevEnabled");
	drawing_menus = false;
	cursor_enabled = false;
	DisableCursor();
	is_first_frame = true;
	freecam = false;
	godmode = false;
	noclip = false;
	post_process_enabled = false;
	if (cheats_enabled) {
		freecam = cfg->getBool("FreecamEnabled");
		godmode = cfg->getBool("GodmodeEnabled");
		noclip = cfg->getBool("NoclipEnabled");
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
    char* oldname = levelFileName;
    UnloadLevel();
    name = AssetPath::clone(name);
    if (!LoadLevel(name)) {
        if (!LoadLevel(AssetPath::level(name))) {
            // if (!LoadLevel(oldname)) {
            //     exit(1);
            // }
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
#pragma region Begin Drawing
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

		GlobalMapData->Draw(camera.position, nullptr, renderScale);
		// DrawPlane({0,0,0}, {5,5}, GRAY);
		GlobalEntityRenderer->Draw(GlobalMapData, camera.position, renderScale);

		EndMode3D();

		EndTextureMode();

		// render the HUD and effects
		BeginTextureMode(screenTexture);

		DrawTexturePro(gameTexture.texture,
			{0, (float)-gameTexture.texture.height, (float)gameTexture.texture.width, (float)-gameTexture.texture.height},
			{0, 0, (float)GetRenderWidth(), (float)GetRenderHeight()},
			{0,0}, 0.0f, WHITE);


		EndTextureMode();

		if (post_process_enabled && IsShaderReady(postShader)) {
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
        sprintf(buffer, "%d", (int)camera.position.x);
        DrawText(buffer, 100, 3, 20, WHITE);
        sprintf(buffer, "%d", (int)camera.position.y);
        DrawText(buffer, 150, 3, 20, WHITE);
        sprintf(buffer, "%d", (int)camera.position.z);
        DrawText(buffer, 200, 3, 20, WHITE);
		DrawFPS(4, 4);

		if (drawing_menus) {
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
	// 			if (is_first_frame) {
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
				if (ImGui::SliderInt("FPS Target", &targetFps, 15, 250)) {
					if (targetFps >= 15) {
						SetTargetFPS(targetFps);
					}
				}
				if (ImGui::Button("Unlimited")) {
					targetFps = -1;
					SetTargetFPS(-1);
				}
				if (ImGui::Checkbox("Fullscreen", &windowIsFullscreen)) {
					ToggleFullscreen();
				}
				if (ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity, 0.05f, 1.0f)) {}
				if (ImGui::SliderFloat("Render Distance", &GlobalMapData->renderDistance, 10.0f, 200.0f)) {}
				if (ImGui::SliderInt("Render Scale", &renderScale, 320, 8192)) {
					ResizeWindow();
				}
				ImGui::Checkbox("Enable Post-processing", &post_process_enabled);
				if (ImGui::Button("Take Screenshot (F2)")) {
					this->TakeScreenshot(screenTexture.texture);
				}
				if (gameTexture.texture.width > 8192 || gameTexture.texture.height > 8192) {
					ImGui::Text("Full-res screen screenshot unavailable (>8192px)");
				} else if (ImGui::Button("Take Full-res Screenshot")) {
					this->TakeScreenshot(gameTexture.texture);
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
				if (ImGui::Checkbox("Freecam", &freecam)) {}
				if (ImGui::Checkbox("Noclip", &noclip)) {}
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
				// lightChanged |= ImGui::SliderFloat("Light value", &dev_lightValue, 0, 255);
				// lightChanged |= ImGui::ColorEdit3("Light Color", dev_lightColor);
				// ImGui::Checkbox("Live Update", &dev_liveUpdateLight);
				// if (dev_liveUpdateLight || ImGui::Button("Set Light")) {
				// 	map->setLight(dev_lightPosition, dev_lightValue, dev_lightColor[2]*255.0f, dev_lightColor[1]*255.0f, dev_lightColor[0]*255.0f);
				// 	lightChanged = false;
				// }
				// ImGui::Checkbox("Live Follow", &dev_liveFollowLight);
				// if (dev_liveFollowLight || ImGui::Button("Set Position")) {
				// 	dev_lightPosition = camera.position;
				// }
				// fillArea |= ImGui::Button("Fill Area");
				// if (fillArea) {
				// 	if (!position1Set) {
				// 		position1Set |= ImGui::Button("Position 1");
				// 		if (position1Set) {
				// 			fillPosition1 = camera.position;
				// 		}
				// 	} else if (!position2Set) {
				// 		position2Set |= ImGui::Button("Position 2");
				// 		if (position2Set) {
				// 			fillPosition2 = camera.position;
				// 			map->setLight(fillPosition1, fillPosition2, dev_lightValue, dev_lightColor[2]*255.0f, dev_lightColor[1]*255.0f, dev_lightColor[0]*255.0f);
				// 			position2Set = position1Set = fillArea = false;
				// 		}
				// 	}
				// }
				ImGui::End();
			}
#pragma endregion


#pragma region End Drawing
			rlImGuiEnd();

            ImGuiIO& io_imgui = ImGui::GetIO();
			if (io_imgui.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
		}

		EndDrawing();
		is_first_frame = false;
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
    if (!cursor_enabled && !is_first_frame) {
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
            adjustedPosition = GlobalMapData->ApplyGravity(adjustedPosition, playerMomentumVertical, dt);
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
#pragma endregion

#pragma region Update
void BR92Engine::Update(float dt) {
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
	CloseWindow();
}
#pragma endregion

#pragma region SaveConfigs
void BR92Engine::SaveConfigs() {
	Vector2 windowPos = GetWindowPosition();
	cfg->setInteger("TargetFPS", targetFps);
	cfg->setUnsigned("WindowSizeX", GetRenderWidth());
	cfg->setUnsigned("WindowSizeY", GetRenderHeight());
	cfg->setInteger("WindowPosX", windowPos.x);
	cfg->setInteger("WindowPosY", windowPos.y + 20);
	cfg->setBool("WindowMaximized", IsWindowMaximized());
	cfg->setBool("WindowFullscreen", IsWindowFullscreen());
    cfg->setFloat("FOVY", camera.fovy);
    cfg->setFloat("PlayerX", camera.position.x);
    cfg->setFloat("PlayerY", camera.position.y);
    cfg->setFloat("PlayerZ", camera.position.z);
    cfg->setFloat("PlayerTX", camera.target.x);
    cfg->setFloat("PlayerTY", camera.target.y);
    cfg->setFloat("PlayerTZ", camera.target.z);
    cfg->setFloat("PlayerUX", camera.up.x);
    cfg->setFloat("PlayerUY", camera.up.y);
    cfg->setFloat("PlayerUZ", camera.up.z);
	cfg->setFloat("RenderDistance", GlobalMapData->renderDistance);
	cfg->setFloat("MouseSensitivity", mouseSensitivity);
	cfg->setBool("CheatsEnabled", cheats_enabled);
	cfg->setBool("FreecamEnabled", freecam);
	cfg->setBool("GodmodeEnabled", godmode);
	cfg->setBool("NoclipEnabled", noclip);
	cfg->setUnsigned("RenderScale", renderScale);
	cfg->save();

	scfg->setByte("FogColorR", GlobalMapData->fogColor[0]*255.0f);
	scfg->setByte("FogColorG", GlobalMapData->fogColor[1]*255.0f);
	scfg->setByte("FogColorB", GlobalMapData->fogColor[2]*255.0f);
	scfg->setByte("FogColorA", GlobalMapData->fogColor[3]*255.0f);
	scfg->setFloat("FogMin", GlobalMapData->fogMin);
	scfg->setFloat("FogMax", GlobalMapData->fogMax);
	scfg->setFloat("LightLevel", GlobalMapData->lightLevel);
	scfg->save();
}
#pragma endregion

#pragma region TakeScreenshot

void BR92Engine::TakeScreenshot(Texture2D texture) {
	char buf[128];
	time_t t = time(nullptr);
	struct tm ts;
	localtime_s(&ts, &t);
	strftime(buf, sizeof(buf), "BR92shot_%Y_%A_%B_%d_%I_%M_%S_%p.png", &ts);
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