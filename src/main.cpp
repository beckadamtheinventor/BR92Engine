
#include "Engine.hpp"
#include "raylib.h"
#include "rcamera.h"
#include "Helpers.hpp"
#include "Engine.hpp"

#define PLAYER_SPEED 1.2f

#pragma region main()
int main(void)
{
	BR92Engine engine;
	SetTraceLogCallback(_logprint);

#pragma region Engine Init

	engine.Init();
	if (!engine.LoadRegistries()) {
		return 1;
	}
	engine.LoadConfigs();
	engine.LoadData();
	engine.OpenWindow((char*)"BR92Engine");
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