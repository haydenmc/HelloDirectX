#include "pch.h"
#include "IInputSource.h"
#include "Simulation.h"

namespace HelloTriangle
{
#pragma region Public
	Simulation::Simulation(
		IInputSource* inputSource
	):
		m_inputSource(inputSource)
	{}

	void Simulation::Update()
	{
		// TODO
	}
#pragma endregion Public
}