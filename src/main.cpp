#include "raylib.h"
#include "rcamera.h"
#include <cstdarg>
#include <fstream>

#pragma region imgui
#include "imgui.h"
#include "rlImGui.h"
#include "imguiThemes.h"
#pragma endregion

#include "SimpleConfig.hpp"
#include "AssetPath.hpp"
#include "MapData.hpp"
#include "TextureRegistry.hpp"
#include "Json.hpp"

#define PLAYER_SPEED 10.0f

const char* MAIN_CONFIG_FILE = "config.dat";
const char* SHADER_CONFIG_FILE = "shader.dat";
const char* VERSION_STRING = "0.0.1-indev";

TextureRegistry GlobalTextureRegistry;
MapTileRegistry GlobalMapTileRegistry;

using namespace SimpleConfig;

std::ofstream __log_fd;

void _logprint(int logLevel, const char* text, va_list args) {
	static std::string logLevels[] = {
		"",
		"TRACE",
		"DEBUG",
		"INFO",
		"WARN",
		"ERROR",
		"FATAL",
		""
	};
	static char buffer[1024];
	int len;
	if (!__log_fd.is_open()) {
		__log_fd.open("debug.log");
	}
	len = snprintf((char*)buffer, sizeof(buffer), "[%s] ", logLevels[logLevel].c_str());
	__log_fd.write(buffer, len);
	len = vsnprintf((char*)buffer, sizeof(buffer), text, args);
	__log_fd.write(buffer, len);
	__log_fd.put('\n');
}

void logprint(int logLevel, const char* text, ...) {
	va_list args;
	va_start(args, text);
	_logprint(logLevel, text, args);
	va_end(args);
}

void JsonFormatError(const char* f, const char* m, const char* s=nullptr) {
	if (s == nullptr) {
		logprint(LOG_ERROR, "Json Format Error in file \"%s\":\n\t%s.\n", f, m);
	} else {
		logprint(LOG_ERROR, "Json Format Error in file \"%s\":\n\t%s. (%s)\n", f, m, s);
	}
	exit(-2);
}

void JsonFormatError(const char* f, const char* m, long long i) {
	logprint(LOG_ERROR, "Json Format Error in file \"%s\":\n\t%s. (%lld)\n", f, m, i);
	exit(-2);
}


void MissingAssetError(const char* f) {
	logprint(LOG_ERROR, "Missing asset file \"%s\"\n", f);
	exit(-1);
}

void AssetFormatError(const char* p) {
	logprint(LOG_ERROR, "format error in asset file \"%s\"", p);
	exit(-3);
}

size_t fstreamlen(std::ifstream& fd) {
	fd.seekg(0, std::ios::end);
	size_t count = fd.tellg();
	fd.seekg(0, std::ios::beg);
	return count;
}
size_t fstreamlen(std::fstream& fd) {
	fd.seekg(0, std::ios::end);
	size_t count = fd.tellg();
	fd.seekg(0, std::ios::beg);
	return count;
}

int main(void)
{
	std::ifstream infd;
	char* datastr;
	SetTraceLogCallback(_logprint);
	// initialize main config object and set defaults
	Config *cfg = new Config();
	cfg->add("ConfigVersion", Value::fromString(VERSION_STRING));
	cfg->add("TargetFPS", Value::fromInteger(-1));
	cfg->add("WindowSizeX", Value::fromUnsigned(640));
	cfg->add("WindowSizeY", Value::fromUnsigned(480));
	cfg->add("WindowPosX", Value::fromInteger(20));
	cfg->add("WindowPosY", Value::fromInteger(20));
	cfg->add("WindowFullscreen", Value::fromBool(false));
	cfg->add("WindowMaximized", Value::fromBool(true));
	cfg->add("MouseSensitivity", Value::fromDouble(0.1f));
	cfg->add("FOVY", Value::fromDouble(60));
	cfg->add("PlayerX", Value::fromDouble(0));
	cfg->add("PlayerY", Value::fromDouble(1));
	cfg->add("PlayerZ", Value::fromDouble(0));
	cfg->add("PlayerTX", Value::fromDouble(1.6));
	cfg->add("PlayerTY", Value::fromDouble(0));
	cfg->add("PlayerTZ", Value::fromDouble(0));
	cfg->add("PlayerUX", Value::fromDouble(0));
	cfg->add("PlayerUY", Value::fromDouble(1));
	cfg->add("PlayerUZ", Value::fromDouble(0));
	cfg->add("RenderDistance", Value::fromDouble(60));

	// Load main config from file
	cfg->deserialize(MAIN_CONFIG_FILE);

	// Initialize shader config and set defaults
	Config* scfg = new Config();
	scfg->add("FogColorR", Value::fromDouble(0.7f));
	scfg->add("FogColorG", Value::fromDouble(0.7f));
	scfg->add("FogColorB", Value::fromDouble(0.7f));
	scfg->add("FogColorA", Value::fromDouble(1.0f));
	scfg->add("FogMin", Value::fromDouble(10.0f));
	scfg->add("FogMax", Value::fromDouble(20.0f));
	scfg->add("LightLevel", Value::fromDouble(1.0f));

	// Load shader config from file
	scfg->deserialize(SHADER_CONFIG_FILE);

	// load map tiles into registry
	RegisteredTexture* nonetex = GlobalTextureRegistry.add("none");
	MapTile* tile = GlobalMapTileRegistry.add("none");
	tile->floor = tile->ceiling = tile->wall = nonetex->id;
	tile->flags = 0;

	infd.open(AssetPath::root("textures", "json"));
	if (infd.is_open()) {
		size_t count = fstreamlen(infd);
		datastr = new char[count+1];
		infd.read(datastr, count);
		datastr[count] = 0;
		infd.close();
		JSON::JSON json = JSON::deserialize(datastr);
		delete [] datastr;
		if (json.contains("elements") && json["elements"].getType() == JSON::Type::Array) {
			JSON::JSONArray& arr = json["elements"].getArray();
			for (size_t i=0; i<arr.length; i++) {
				if (arr[i].getType() == JSON::Type::String) {
					GlobalTextureRegistry.add(arr[i].getCString());
				}
			}
		} else {
			JsonFormatError("textures.json", "Expected member \"elements\" in root containing an array of strings");
		}
	} else {
		MissingAssetError(AssetPath::root("textures", "json"));
	}

	infd.open(AssetPath::root("tiles", "json"));
	if (infd.is_open()) {
		size_t count = fstreamlen(infd);
		datastr = new char[count+1];
		infd.read(datastr, count);
		datastr[count] = 0;
		infd.close();
		JSON::JSON json = JSON::deserialize(datastr);
		delete [] datastr;
		if (json.contains("elements") && json["elements"].getType() == JSON::Type::Array) {
			JSON::JSONArray& arr = json["elements"].getArray();
			for (size_t i=0; i<arr.length; i++) {
				if (arr[i].getType() == JSON::Type::Object) {
					JSON::JSONObject& o = arr[i].getObject();
					const char* id;
					if (o.has("id") && o["id"].getType() == JSON::Type::String) {
						id = o["id"].getCString();
					} else {
						JsonFormatError("tiles.json", "Elements array contains invalid member (missing id)");
					}
					MapTile* tile = GlobalMapTileRegistry.add(id);
					tile->light = 0;
					if (o.has("f")) {
						tile->isSpawnable = true;
						tile->isSolid = false;
						tile->isWall = false;
						tile->blocksLight = false;
						if (o["f"].getType() == JSON::Type::String) {
							const char* f = o["f"].getCString();
							if (GlobalTextureRegistry.has(f)) {
								tile->floor = GlobalTextureRegistry.of(f)->id;
							} else {
								JsonFormatError("tiles.json", "Elements array member contains unknown texture id", f);
							}
						} else if (o["f"].getType() == JSON::Type::Integer) {
							long long i = o["f"].getInteger();
							if (i >= 0 && i < 65536) {
								if (GlobalTextureRegistry.has(i)) {
									tile->floor = i;
								} else {
									JsonFormatError("tiles.json", "Elements array member contains unknown tile id number", i);
								}
							} else {
								JsonFormatError("tiles.json", "Elements array member contains out of bound tile id number", i);
							}
						} else {
							JsonFormatError("tiles.json", "Elements array member contains invalid value for field", "f");
						}
					}
					if (o.has("c")) {
						tile->isSolid = false;
						tile->isWall = false;
						tile->blocksLight = false;
						if (o["c"].getType() == JSON::Type::String) {
							const char* f = o["c"].getCString();
							if (GlobalTextureRegistry.has(f)) {
								tile->ceiling = GlobalTextureRegistry.of(f)->id;
							} else {
								JsonFormatError("tiles.json", "Elements array member contains unknown texture id", f);
							}
						} else if (o["c"].getType() == JSON::Type::Integer) {
							long long i = o["c"].getInteger();
							if (i >= 0 && i < 65536) {
								if (GlobalTextureRegistry.has(i)) {
									tile->ceiling = i;
								} else {
									JsonFormatError("tiles.json", "Elements array member contains unknown tile id number", i);
								}
							} else {
								JsonFormatError("tiles.json", "Elements array member contains out of bound tile id number", i);
							}
						} else {
							JsonFormatError("tiles.json", "Elements array member contains invalid value type for field", "c");
						}
					}
					if (o.has("w")) {
						tile->isWall = true;
						tile->isSolid = true;
						tile->blocksLight = true;
						if (o["w"].getType() == JSON::Type::String) {
							const char* f = o["w"].getCString();
							if (GlobalTextureRegistry.has(f)) {
								tile->wall = GlobalTextureRegistry.of(f)->id;
							} else {
								JsonFormatError("tiles.json", "Elements array member contains unknown texture id", f);
							}
						} else if (o["w"].getType() == JSON::Type::Integer) {
							long long i = o["w"].getInteger();
							if (i >= 0 && i < 65536) {
								if (GlobalTextureRegistry.has(i)) {
									tile->wall = i;
								} else {
									JsonFormatError("tiles.json", "Elements array member contains unknown tile id number", i);
								}
							} else {
								JsonFormatError("tiles.json", "Elements array member contains out of bound tile id number", i);
							}
						} else {
							JsonFormatError("tiles.json", "Elements array member contains invalid value type (should be integer or string) for field", "w");
						}
					}
					if (o.has("solid")) {
						if (o["solid"].getType() == JSON::Type::Boolean) {
							tile->isSolid = o["solid"].getBoolean();
						} else {
							JsonFormatError("tiles.json", "Elements array member contains invalid value type (should be bool) for field", "solid");
						}
					}
					if (o.has("spawnable")) {
						if (o["spawnable"].getType() == JSON::Type::Boolean) {
							tile->isSpawnable = o["spawnable"].getBoolean();
						} else {
							JsonFormatError("tiles.json", "Elements array member contains invalid value type (should be bool) for field", "spawnable");
						}
					}
					if (o.has("wall")) {
						if (o["wall"].getType() == JSON::Type::Boolean) {
							tile->isSolid = o["wall"].getBoolean();
						} else {
							JsonFormatError("tiles.json", "Elements array member contains invalid value type (should be bool) for field", "wall");
						}
					}
					if (o.has("blockslight")) {
						if (o["blockslight"].getType() == JSON::Type::Boolean) {
							tile->blocksLight = o["blockslight"].getBoolean();
						} else {
							JsonFormatError("tiles.json", "Elements array member contains invalid value type (should be bool) for field", "blockslight");
						}
					}
					if (o.has("light")) {
						if (o["light"].getType() == JSON::Type::Integer) {
							tile->light = o["light"].getInteger();
						} else {
							JsonFormatError("tiles.json", "Elements array mamber contains invalid value type (should be integer) for field", "light");
						}
					}
					if (o.has("tint")) {
						if (o["tint"].getType() == JSON::Type::Array) {
							JSON::JSONArray& arr = o["tint"].getArray();
							if (arr.length != 3) {
								JsonFormatError("tiles.json", "Elements array member conatins invalid value type (should be 3-component integer array) for field", "tint");
							}
							if (arr.members[0].getType() != JSON::Type::Integer ||
								arr.members[1].getType() != JSON::Type::Integer ||
								arr.members[2].getType() != JSON::Type::Integer) {
									JsonFormatError("tiles.json", "Elements array member conatins invalid value type (should be 3-component integer array) for field", "tint");
							}
							tile->tintr = arr.members[0].getInteger();
							tile->tintg = arr.members[1].getInteger();
							tile->tintb = arr.members[2].getInteger();
						} else {
							JsonFormatError("tiles.json", "Elements array mamber contains invalid value type (should be 3-component integer array) for field", "tint");
						}
					} else {
						tile->tintr = 255;
						tile->tintg = 255;
						tile->tintb = 255;
					}
				}
			}
		} else {
			JsonFormatError("tiles.json", "Expected member \"elements\" in root containing an array of objects");
		}
	} else {
		MissingAssetError(AssetPath::root("tiles", "json"));
	}

	infd.open(AssetPath::root("index", nullptr));
	if (infd.is_open()) {
		size_t count = fstreamlen(infd);
		datastr = new char[count+1];
		infd.read(datastr, count);
		datastr[count] = 0;
		infd.close();
	} else {
		MissingAssetError(AssetPath::root("index", nullptr));
	}

	MapData map;
	map.TextureRegistry(&GlobalTextureRegistry);
	map.TileRegistry(&GlobalMapTileRegistry);
	RBuffer<char> readbuf;
	readbuf.open(AssetPath::level(datastr));
	if (readbuf.available() > 0) {
		if (!map.LoadMap(readbuf)) {
			AssetFormatError(AssetPath::level(datastr));
		}
	} else {
		MissingAssetError(AssetPath::level(datastr));
	}
	delete [] datastr;


	SetConfigFlags(FLAG_WINDOW_RESIZABLE);

	InitWindow(320, 240, "TheBackrooms1992");

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
	int targetFps = cfg->getInteger("TargetFPS");
	if (targetFps != -1 && targetFps < 15) {
		targetFps = 15;
	}
	float mouseSensitivity = cfg->getDouble("MouseSensitivity");

	SetWindowMinSize(320, 240);
	SetTargetFPS(targetFps);
	SetExitKey(-1);

#pragma region imgui
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

#pragma endregion

	RenderTexture2D gameTexture = LoadRenderTexture(1920, 1080);

	map.BuildAtlas();
	map.InitMesher(gameTexture.depth.id);
	map.BuildLighting();
	map.GenerateMesh();
	map.UploadMap();

	Camera3D camera = {
		{0,1.0,0},
		{1.6, 0, 0},
		{0, 1.0, 0},
		60, CAMERA_PERSPECTIVE
	};

	camera.fovy = cfg->getDouble("FOVY");
	camera.position.x = cfg->getDouble("PlayerX");
	camera.position.y = cfg->getDouble("PlayerY");
	camera.position.z = cfg->getDouble("PlayerZ");
	camera.target.x = cfg->getDouble("PlayerTX");
	camera.target.y = cfg->getDouble("PlayerTY");
	camera.target.z = cfg->getDouble("PlayerTZ");
	camera.up.x = cfg->getDouble("PlayerUX");
	camera.up.y = cfg->getDouble("PlayerUY");
	camera.up.z = cfg->getDouble("PlayerUZ");
	map.renderDistance = cfg->getDouble("RenderDistance");

	map.fogColor[0] = scfg->getDouble("FogColorR");
	map.fogColor[1] = scfg->getDouble("FogColorG");
	map.fogColor[2] = scfg->getDouble("FogColorB");
	map.fogColor[3] = scfg->getDouble("FogColorA");
	map.fogMin = scfg->getDouble("FogMin");
	map.fogMax = scfg->getDouble("FogMax");
	map.lightLevel = scfg->getDouble("LightLevel");

	io_imgui.WantCaptureKeyboard = false;
	io_imgui.WantCaptureMouse = false;

	bool cursor_enabled = true;
	bool is_first_frame = true;
	ImVec2 gameWindowPosition;

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(BLACK);

		BeginTextureMode(gameTexture);
		{
			Color tmp = {
				(unsigned char)(map.fogColor[0]*255.0f),
				(unsigned char)(map.fogColor[1]*255.0f),
				(unsigned char)(map.fogColor[2]*255.0f),
				(unsigned char)(map.fogColor[3]*255.0f)
			};
			ClearBackground(tmp);
		}
		BeginMode3D(camera);

		map.Draw(camera.position);
		// DrawPlane({0,0,0}, {5,5}, GRAY);

		EndMode3D();

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

		EndTextureMode();

	#pragma region imgui
		rlImGuiBegin();

		// ImGui::PushStyleColor(ImGuiCol_WindowBg, {});
		// ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, {});
		// ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
		// ImGui::PopStyleColor(2);

		/* Game Window */
		{
			ImGui::Begin("Game View", nullptr, ImGuiViewportFlags_NoRendererClear|ImGuiWindowFlags_NoCollapse);
			gameWindowPosition = ImGui::GetWindowPos();
			if (is_first_frame) {
				ImGui::SetWindowSize({1920*0.75f, 1080*0.75f});
				ImGui::SetWindowPos({0,0});
			} else {
				ImVec2 wsize = ImGui::GetWindowSize();
				if (wsize.x < 128) {
					wsize.x = 128;
				}
				if (wsize.y < 128) {
					wsize.y = 128;
				}
				ImGui::SetWindowSize(wsize);
				gameWindowPosition.x += wsize.x/2;
				gameWindowPosition.y += wsize.y/2;
			}
			rlImGuiImageRenderTextureFit(&gameTexture, true);
			ImGui::End();
		}


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
			if (ImGui::SliderFloat("Render Distance", &map.renderDistance, 10.0f, 200.0f)) {}
			ImGui::End();
		}

		/* Shader Config Menu */
		{
			ImGui::Begin("Shader Config");
			if (ImGui::ColorEdit4("Fog Color", map.fogColor)) {}
			if (ImGui::InputFloat("Fog Min", &map.fogMin)) {}
			if (ImGui::InputFloat("Fog Max", &map.fogMax)) {}
			if (ImGui::InputFloat("Light", &map.lightLevel)) {}
			ImGui::End();
		}

		rlImGuiEnd();

		if (io_imgui.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	#pragma endregion

		EndDrawing();

		float dt = GetFrameTime();

		if (!cursor_enabled) {
			Vector3 movement = {0, 0, 0};
			Vector2 mouseDelta = GetMouseDelta();
			SetMousePosition(gameWindowPosition.x, gameWindowPosition.y);
			float delta = PLAYER_SPEED*dt;
			if (IsKeyDown(KEY_LEFT_CONTROL)) {
				delta *= 1.4f;
			}
			if (IsKeyDown(KEY_W)) {
				CameraMoveForward(&camera, delta, true);
			}
			if (IsKeyDown(KEY_S)) {
				CameraMoveForward(&camera, -delta, true);
			}
			if (IsKeyDown(KEY_A)) {
				CameraMoveRight(&camera, -delta, true);
			}
			if (IsKeyDown(KEY_D)) {
				CameraMoveRight(&camera, delta, true);
			}
			CameraYaw(&camera, -mouseDelta.x*dt*mouseSensitivity, false);
			CameraPitch(&camera, -mouseDelta.y*dt*mouseSensitivity, true, false, false);
		}

		if (IsKeyPressed(KEY_ESCAPE)) {
			if (cursor_enabled) {
				cursor_enabled = false;
				DisableCursor();
			} else {
				cursor_enabled = true;
				EnableCursor();
			}
		}

		is_first_frame = false;
	}


#pragma region imgui
	rlImGuiShutdown();
#pragma endregion



	CloseWindow();

	Vector2 windowPos = GetWindowPosition();
	cfg->setBool("WindowMaximized", IsWindowMaximized());
	cfg->setBool("WindowFullscreen", IsWindowFullscreen());
	cfg->setInteger("TargetFPS", targetFps);
	cfg->setInteger("WindowPosX", windowPos.x);
	cfg->setInteger("WindowPosY", windowPos.y + 20);
	cfg->setUnsigned("WindowSizeX", GetRenderWidth());
	cfg->setUnsigned("WindowSizeY", GetRenderHeight());
	cfg->setDouble("MouseSensitivity", mouseSensitivity);
	cfg->setDouble("FOVY", camera.fovy);
	cfg->setDouble("PlayerX",  camera.position.x);
	cfg->setDouble("PlayerY",  camera.position.y);
	cfg->setDouble("PlayerZ",  camera.position.z);
	cfg->setDouble("PlayerTX",  camera.target.x);
	cfg->setDouble("PlayerTY",  camera.target.y);
	cfg->setDouble("PlayerTZ",  camera.target.z);
	cfg->setDouble("PlayerUX",  camera.up.x);
	cfg->setDouble("PlayerUY",  camera.up.y);
	cfg->setDouble("PlayerUZ",  camera.up.z);
	cfg->setDouble("RenderDistance", map.renderDistance);
	cfg->serialize(MAIN_CONFIG_FILE);

	scfg->setDouble("FogColorR", map.fogColor[0]);
	scfg->setDouble("FogColorG", map.fogColor[1]);
	scfg->setDouble("FogColorB", map.fogColor[2]);
	scfg->setDouble("FogColorA", map.fogColor[3]);
	scfg->setDouble("FogMin", map.fogMin);
	scfg->setDouble("FogMax", map.fogMax);
	scfg->setDouble("LightLevel", map.lightLevel);
	scfg->serialize(SHADER_CONFIG_FILE);

	__log_fd.close();
	return 0;
}