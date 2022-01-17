#pragma once
#include <cstdint>

namespace HelloTriangle
{
	/// <summary>
	/// IInputSource represents a class that can provide input information
	/// </summary>
	class IInputSource
	{
	public:
		virtual void GetKeyState() = 0;
	};
}