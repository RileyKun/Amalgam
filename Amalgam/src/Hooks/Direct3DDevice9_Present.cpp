#include "Direct3DDevice9_Present.h"

#include "../SDK/SDK.h"
#include "../Features/ImGui/Render.h"
#include "../Features/ImGui/Menu/Menu.h"

#include <wrl.h>

#pragma comment (lib, "d3d9.lib") // We'll get an unresolved external if we do not have this.

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

LONG __stdcall WndProc::Func(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (F::Menu.IsOpen)
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
		if (ImGui::GetIO().WantTextInput || F::Menu.InKeybind)
		{
			I::InputSystem->ResetInputState();
			return 1;
		}

		if (uMsg >= WM_MOUSEFIRST && WM_MOUSELAST >= uMsg)
			return 1;
	}

	return CallWindowProc(Original, hWnd, uMsg, wParam, lParam);
}

MAKE_HOOK(VGuiSurface_LockCursor, U::Memory.GetVFunc(I::MatSystemSurface, 62), void, __fastcall,
	void* ecx)
{
	if (F::Menu.IsOpen)
		return I::MatSystemSurface->UnlockCursor();

	CALL_ORIGINAL(ecx);
}

MAKE_HOOK(VGuiSurface_SetCursor, U::Memory.GetVFunc(I::MatSystemSurface, 51), void, __fastcall,
	void* ecx, HCursor cursor)
{
	if (F::Menu.IsOpen)
	{
		auto iCursor = F::Render.m_iCursor.load();
		switch(iCursor)
		{
		case 0: cursor = 2; break;
		case 1: cursor = 3; break;
		case 2: cursor = 12; break;
		case 3: cursor = 11; break;
		case 4: cursor = 10; break;
		case 5: cursor = 9; break;
		case 6: cursor = 8; break;
		case 7: cursor = 14; break;
		case 8: cursor = 13; break;
		}
		return CALL_ORIGINAL(ecx, cursor);
	}

	CALL_ORIGINAL(ecx, cursor);
}

void WndProc::Initialize()
{
	while (!hwWindow)
		hwWindow = SDK::GetTeamFortressWindow();

	Original = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hwWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Func)));
}

void WndProc::Unload()
{
	SetWindowLongPtr(hwWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Original));
}

void DirectX::Startup()
{
	// Create a temporary Direct3D object.
	ComPtr<IDirect3D9> pD3D9 = Direct3DCreate9(D3D_SDK_VERSION);
	auto hwWindow = FindWindowA("Valve001", nullptr);

	D3DPRESENT_PARAMETERS Params{
		.BackBufferFormat = D3DFMT_UNKNOWN,
		.SwapEffect = D3DSWAPEFFECT_DISCARD,
		.Windowed = TRUE,
		.EnableAutoDepthStencil = TRUE,
		.AutoDepthStencilFormat = D3DFMT_D16,
		.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT,
	};
	
	// Create a temporary device, this will be replaced with the game's device later.
	ComPtr<IDirect3DDevice9> pDevice{};
	pD3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING, &Params, &pDevice);

	MH_CreateHook(U::Memory.GetVFunc(pDevice.Get(), 17), &HookedPresent, (void**)&pOriginalPresent);
	MH_CreateHook(U::Memory.GetVFunc(pDevice.Get(), 16), &HookedReset, (void**)&pOriginalReset);
}

HRESULT __stdcall DirectX::HookedPresent(IDirect3DDevice9* pDevice, const RECT* pSource, const RECT* pDestination, const RGNDATA* pDirtyRegion)
{
	// Do not render if the cheat itself is not ready to do so.
	// Access violations are no joke...
	if(!bIsReady.load()) 
		return pOriginalPresent(pDevice, pSource, pDestination, pDirtyRegion); 

	F::Render.Render(pDevice);
	return pOriginalPresent(pDevice, pSource, pDestination, pDirtyRegion);
}

HRESULT __stdcall DirectX::HookedReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pParams)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	pOriginalReset(pDevice, pParams);
	ImGui_ImplDX9_CreateDeviceObjects();
	return S_OK;
}
