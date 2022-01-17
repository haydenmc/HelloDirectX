#include "pch.h"
#include "Window.h"

namespace HelloTriangle
{
#pragma region Public
	Window::Window(
		HINSTANCE hInstance,
		std::string title,
		std::string windowClass,
		uint32_t width,
		uint32_t height
	) :
		m_hInstance(hInstance),
		m_title(title),
		m_windowClass(windowClass),
		m_width(width),
		m_height(height)
	{
		RegisterWindowClass();
	}

	void Window::Initialize()
	{
		CreateHwnd();
		if (m_handle == nullptr)
		{
			throw std::exception("Could not create window!");
		}

		ShowWindow(m_handle, SW_SHOW);
		SetForegroundWindow(m_handle);
		SetFocus(m_handle);
	}

	uint32_t Window::GetWidth()
	{
		return m_width;
	}

	uint32_t Window::GetHeight()
	{
		return m_height;
	}

	HWND Window::GetHwnd()
	{
		return m_handle;
	}

	MessagePumpResult Window::PumpMessages()
	{
		MSG msg{ 0 };
		if (PeekMessageW(&msg, m_handle, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		if (msg.message == WM_NULL)
		{
			if (!IsWindow(m_handle))
			{
				m_handle = nullptr;
				std::wstring windowClass{ m_windowClass.begin(), m_windowClass.end() };
				UnregisterClassW(windowClass.c_str(), m_hInstance);
				return MessagePumpResult::Closed;
			}
		}

		return MessagePumpResult::Continue;
	}

	void Window::GetKeyState()
	{
		// TODO
	}
#pragma endregion Public

#pragma region Private
	void Window::RegisterWindowClass()
	{
		spdlog::debug("Window: Registering Window Class...");
		std::wstring className{ m_windowClass.begin(), m_windowClass.end() };
		WNDCLASSEXW windowClass
		{
			.cbSize = sizeof(WNDCLASSEXW),
			.style = (CS_HREDRAW | CS_VREDRAW | CS_OWNDC),
			.lpfnWndProc = WindowProc,
			.cbClsExtra = 0,
			.cbWndExtra = 0,
			.hInstance = m_hInstance,
			.hIcon = nullptr,
			.hCursor = LoadCursorW(nullptr, IDC_ARROW),
			.hbrBackground = nullptr,
			.lpszMenuName = nullptr,
			.lpszClassName = className.c_str(),
			.hIconSm = nullptr,
		};
		RegisterClassExW(&windowClass);
	}

	void Window::CreateHwnd()
	{
		if (m_handle != nullptr)
		{
			spdlog::warn("Window: CreateHwnd unexpectedly called multiple times.");
			return;
		}

		spdlog::info("Window: Creating new HWND");
		std::wstring windowClass{ m_windowClass.begin(), m_windowClass.end() };
		std::wstring title{ m_title.begin(), m_title.end() };
		m_handle = CreateWindowExW(
			0,
			windowClass.c_str(),
			title.c_str(),
			(WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU),
			0,
			0,
			m_width,
			m_height,
			nullptr,
			nullptr,
			m_hInstance,
			this
		);
	}

	LRESULT CALLBACK Window::WindowProc(
		HWND hwnd,
		UINT msg,
		WPARAM wParam,
		LPARAM lParam
	)
	{
		// Maintain reference to Window instance
		Window* that{ nullptr };

		if (msg == WM_NCCREATE)
		{
			// If the window was just created, translate the Window pointer
			// from creation parameter to the hwnd userdata
			that = static_cast<Window*>(
				reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
			SetLastError(0);
			if (!SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that)))
			{
				if (GetLastError() != 0)
				{
					throw std::exception("Could not set userdata on hwnd!");
				}
			}
		}
		else
		{
			// Otherwise, extract the Window pointer from the hwnd userdata.
			that = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		if (that)
		{
			return that->InstanceWindowProc(hwnd, msg, wParam, lParam);
		}
		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}

	LRESULT CALLBACK Window::InstanceWindowProc(
		HWND hwnd,
		UINT msg,
		WPARAM wParam,
		LPARAM lParam
	)
	{
		switch (msg)
		{
		case WM_KEYDOWN:
			// TODO
			return 0;

		case WM_KEYUP:
			// TODO
			return 0;

		case WM_PAINT:
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		// Handle messages if we didn't
		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}
#pragma endregion Private
}