#include "pch.h"
#include "Simulation.h"

#include "RenderWindow.h"

namespace HelloTriangle
{
#pragma region Public
	Simulation::Simulation(
		RenderWindow* window
	):
		m_window(window),
		m_renderer(std::make_unique<Renderer>(window))
	{}

	void Simulation::Run()
	{
		MessagePumpResult result{ MessagePumpResult::Continue };

		while (result != MessagePumpResult::Closed)
		{
			result = m_window->PumpMessages();
		}
	}

	void Simulation::OnUpdate()
	{
		// TODO
	}

	void Simulation::OnRender()
	{
		// TODO
	}

	void Simulation::OnKeyDown(uint8_t keyCode)
	{
		// TODO
	}

	void Simulation::OnKeyUp(uint8_t keyCode)
	{
		// TODO
	}
#pragma endregion Public
}