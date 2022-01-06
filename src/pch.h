#pragma once

// Windows headers
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>

// DirectX headers
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

// D3D12 Extension Librayr
#include <d3dx12.h>

// STL
#include <algorithm>
#include <cassert>
#include <chrono>
#include <string>

// Spdlog
#include <spdlog/spdlog.h>

// Common program headers
#include "Utility.h"

namespace MWRL = Microsoft::WRL;