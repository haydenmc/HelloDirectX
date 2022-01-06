#pragma once
#include "IInputSink.h"
#include "Renderer.h"

#include <memory>

namespace HelloTriangle
{
	class RenderWindow;

	/// <summary>
	/// The Simulation class manages the main loop and various subsystems (input, graphics, etc.)
	/// </summary>
	class Simulation : public IInputSink
	{
	public:
		Simulation(RenderWindow* window);
		void Run();
		void OnUpdate();
		void OnRender();

		// IInputSink
		virtual void OnKeyDown(uint8_t keyCode);
		virtual void OnKeyUp(uint8_t keyCode);

	private:
		RenderWindow* const m_window;
		std::unique_ptr<Renderer> m_renderer;
	};
}