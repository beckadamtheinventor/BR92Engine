
#include "Engine.hpp"
#include "raylib.h"
#include "rcamera.h"
#include "Helpers.hpp"

#define WIN_TITLE (char*)"BR92Engine"

#pragma region main()
int main(int argc, char** argv)
{
#ifdef VR_SUPPORT
	int boot_in_vr = -1;
#endif
	BR92Engine engine;
	SetTraceLogCallback(_logprint);

	for (int i=1; i<argc; i++) {
#ifdef VR_SUPPORT
		if (!strcmp(argv[i], "--vr") || !strcmp(argv[i], "-v")) {
			boot_in_vr = 1;
		}
		if (!strcmp(argv[i], "--desktop") || !strcmp(argv[i], "-d")) {
			boot_in_vr = 0;
		}
#endif
	}

#pragma region Engine Init

	engine.Init();
	if (!engine.LoadRegistries()) {
		return 1;
	}
	engine.LoadConfigs();
	engine.LoadData();
#ifdef VR_SUPPORT
	if (boot_in_vr == 0) {
#endif
		engine.OpenWindow(WIN_TITLE);
#ifdef VR_SUPPORT
	} else {
		engine.OpenWindowVR(WIN_TITLE);
	}
#endif
	engine.InitMesher();
	engine.InitCamera();
	engine.InitImGui();

#pragma endregion

#pragma region Main Loop

	if (!engine.LoadLevel()) {
		return -1;
	}
	engine.BeforeMainLoop();

	while (!WindowShouldClose())
	{
		engine.Draw();
		float dt = GetFrameTime();
		engine.HandleInputs(dt);
		engine.Update(dt);
	}

#pragma endregion

	engine.EndWindow();

	CloseLog();
	return 0;
}
#pragma endregion