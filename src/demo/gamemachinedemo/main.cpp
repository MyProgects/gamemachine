﻿#include <iostream>
#include <gamemachine.h>
#include <gmm.h>
#include "demonstration_world.h"
#include <wrapper.h>

namespace
{
	gm::GMRenderEnvironment s_env;

	void SetRenderEnv(gm::GMRenderEnvironment env)
	{
		s_env = env;
	}
}

gm::GMRenderEnvironment GetRenderEnv()
{
	return s_env;
}

#if GM_WINDOWS
int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine,
	int nCmdShow
)
{
	LPWSTR cmdLine = GetCommandLineW();
#elif GM_UNIX
int main(int argc, char* argv[])
{	
#endif

#if GM_WINDOWS
	gm::GMsize_t sz = gm::GMString::countOfCharacters(cmdLine);
	gm::GMRenderEnvironment env = gm::GMRenderEnvironment::DirectX11;
	if (sz > 7 && gm::GMString(cmdLine + sz - 7) == "-opengl")
		env = gm::GMRenderEnvironment::OpenGL;
#elif GM_UNIX
	gm::GMRenderEnvironment env = gm::GMRenderEnvironment::OpenGL;
	gm::GMInstance hInstance = 0; // Useless in unix
#endif

	gm::IFactory* factory = nullptr;
	env = GMCreateFactory(env, gm::GMRenderEnvironment::OpenGL, &factory);
	GM_ASSERT(env != gm::GMRenderEnvironment::Invalid && factory);
	SetRenderEnv(env);

	gm::GMGameMachineDesc desc;
	desc.factory = factory;
	desc.renderEnvironment = GetRenderEnv();
	GM.init(desc);

	gm::GMWindowDesc windowDesc;
	windowDesc.instance = hInstance;

#if GM_WINDOWS
	windowDesc.dwStyle = WS_OVERLAPPEDWINDOW | WS_SIZEBOX;
#endif
	// windowDesc.samples = 1;

	gm::IWindow* mainWindow = nullptr;
	factory->createWindow(hInstance, nullptr, &mainWindow);
	mainWindow->create(windowDesc);
	mainWindow->centerWindow();
	mainWindow->showWindow();
	mainWindow->setHandler(new DemostrationEntrance(mainWindow));
	GM.addWindow(mainWindow);

	/*
	gm::IWindow* mainWindow2 = nullptr;
	windowDesc.windowName = "Window 2";
	factory->createWindow(hInstance, mainWindow, &mainWindow2);
	mainWindow2->create(windowDesc);
	mainWindow2->centerWindow();
	mainWindow2->showWindow();
	mainWindow2->setHandler(new DemostrationEntrance(mainWindow2));
	GM.addWindow(mainWindow2);
	*/

	GM.startGameMachine();
	return 0;
}
