#pragma once
#include <wtypes.h>
#include <string>

namespace HelloTriangle
{
	class IInputSink;
	class Simulation;

	enum class MessagePumpResult
	{
		Continue,
		Closed,
	};

	class RenderWindow
	{
	public:
		RenderWindow(
			HINSTANCE hInstance,
			std::string title,
			std::string windowClass,
			uint32_t width,
			uint32_t height);

		void Initialize(
			Simulation* simulation,
			IInputSink* inputSink);
		uint32_t GetWidth();
		uint32_t GetHeight();
		HWND GetHwnd();
		MessagePumpResult PumpMessages();

	private:
		HWND m_handle{ nullptr };
		const HINSTANCE m_hInstance;
		const std::string m_title;
		const std::string m_windowClass;
		const uint32_t m_width;
		const uint32_t m_height;

		Simulation* m_simulation;
		IInputSink* m_inputSink;

		void RegisterWindowClass();
		void CreateHwnd();
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK InstanceWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	};
}