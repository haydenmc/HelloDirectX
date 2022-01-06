#pragma once
#include "pch.h"
#include "RenderWindow.h"
#include <memory>

namespace HelloTriangle
{
	class Renderer
	{
	public:
		Renderer(RenderWindow* window, bool useWarpDevice = false);

		void OnInit();
		void OnUpdate();
		void OnRender();
		void OnDestroy();

	private:
		static constexpr int NUM_FRAMES = 2;
		RenderWindow* const m_window;
		const bool m_useWarpDevice;

		// DX Pipeline
		Microsoft::WRL::ComPtr<ID3D12Device> m_d3dDevice;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
		std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, NUM_FRAMES> m_renderTargets;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
		uint32_t m_rtvDescriptorSize{ 0 };

		// Synchronization
		uint32_t m_frameIndex{ 0 };
		HANDLE m_fenceEvent{ nullptr };
		Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
		uint64_t m_fenceValue{ 0 };

		void LoadPipeline();
		void LoadAssets();
		void PopulateCommandList();
		void WaitForPreviousFrame();

		void GetHardwareAdapter(
			IDXGIFactory1* pFactory,
			IDXGIAdapter1** ppAdapter,
			bool requestHighPerformanceAdapter = false);
	};
}