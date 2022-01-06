#pragma once
#include <cstdint>

namespace HelloTriangle
{
	/// <summary>
	/// IInputSink represents a class that can receive input callbacks
	/// </summary>
	class IInputSink
	{
	public:
		virtual void OnKeyDown(uint8_t keyCode) = 0;
		virtual void OnKeyUp(uint8_t keyCode) = 0;
	};
}