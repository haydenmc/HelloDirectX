#include "pch.h"
#include "Renderer.h"
#include "Window.h"
#include "Simulation.h"

#include <memory>

int wmain(int argc, wchar_t* argv[])
{
#if _DEBUG
	spdlog::set_level(spdlog::level::debug);
#endif

	std::unique_ptr<HelloTriangle::Simulation> simulation{ nullptr };
	std::unique_ptr<HelloTriangle::Window> window{ nullptr };
	std::unique_ptr<HelloTriangle::Renderer> renderer{ nullptr };

	bool isRenderEnabled = true;

	if (isRenderEnabled)
	{
		spdlog::info("Main: Creating window...");
		HINSTANCE hInstance{ GetModuleHandleW(nullptr) };
		window = std::make_unique<HelloTriangle::Window>(
			hInstance,
			"Hello World",
			"MyWindowClass",
			800,
			600
		);
		spdlog::info("Main: Initializing window...");
		window->Initialize();

		spdlog::info("Main: Creating renderer...");
		renderer = std::make_unique<HelloTriangle::Renderer>(window.get(), 800, 600);
		spdlog::info("Main: Initializing renderer...");
		renderer->Initialize();
	}
	else
	{
		spdlog::info("Main: Rendering is disabled, skipping renderer and window initialization.");
	}

	// Simulation is always initialized, even if we aren't rendering
	spdlog::info("Main: Creating Simulation...");
	simulation = std::make_unique<HelloTriangle::Simulation>(window.get());
	
	// Game loop
	spdlog::info("Main: Starting main loop...");
	while (true)
	{
		if (window)
		{
			HelloTriangle::MessagePumpResult windowResult = window->PumpMessages();
			if (windowResult == HelloTriangle::MessagePumpResult::Closed)
			{
				// The window has been closed - terminate the game loop
				spdlog::info("Main: Window closed, breaking main loop...");
				break;
			}
		}

		simulation->Update();

		if (renderer)
		{
			renderer->Render();
		}
	}
	spdlog::info("Main: Main loop terminated.");
}