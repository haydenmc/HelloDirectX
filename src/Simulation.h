#pragma once

namespace HelloTriangle
{
	class IInputSource;

	/// <summary>
	/// The Simulation class manages the main loop and various subsystems (input, graphics, etc.)
	/// </summary>
	class Simulation
	{
	public:
		Simulation(IInputSource* inputSource);
		void Update();

	private:
		IInputSource* const m_inputSource{ nullptr };
	};
}