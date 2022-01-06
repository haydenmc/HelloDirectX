#include "pch.h"
#include "RenderWindow.h"
#include "Simulation.h"

#include <memory>

int wmain(int argc, wchar_t* argv[])
{
#if _DEBUG
	spdlog::set_level(spdlog::level::debug);
#endif
	HINSTANCE hInstance{ GetModuleHandleW(nullptr) };
	HelloTriangle::RenderWindow window
	{
		hInstance,
		"Hello World",
		"MyWindowClass",
		800,
		600
	};

	std::unique_ptr<HelloTriangle::Simulation> simulation =
		std::make_unique<HelloTriangle::Simulation>(&window);

	window.Initialize(
		simulation.get(), // Simulation
		simulation.get() // IInputSink
	);

	spdlog::info("Main: Running simulation...");
	simulation->Run();
	spdlog::info("Main: Simulation ended.");
}