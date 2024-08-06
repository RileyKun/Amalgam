#include "Core.h"

#include "../SDK/SDK.h"
#include "../Features/Visuals/Materials/Materials.h"
#include "../Features/Configs/Configs.h"
#include "../Features/Commands/Commands.h"
#include "../Features/ImGui/Menu/Menu.h"
#include "../Features/Visuals/Visuals.h"
#include "../Hooks/Direct3DDevice9_Present.h"

__forceinline bool CheckRenderIsCompatible()
{
	// Check for if we are running in DXVK mode.
	if(GetModuleHandleA("dxvk_d3d9.dll"))
	{
		MessageBoxA(nullptr, "You are running with graphics options that Amalgam does not support.\n"
			"Please remove -vulkan from your launch options and reinject.", "Error", MB_ICONERROR);
		U::Core.bUnload = true;
		return false;
	}

	// Check for if we are running in DirectX 8.
	auto cvDXLevel = I::CVar->FindVar("mat_dxlevel");
	auto iLevel = cvDXLevel->GetInt();
	if(iLevel < 90)
	{
		std::string fmt = std::format("You are running with graphics options that Amalgam does not support.\nPlease remove -dxlevel {} from your launch options and reinject.", iLevel);
		MessageBox(nullptr, fmt.c_str(), "Error", MB_ICONERROR);
		U::Core.bUnload = true;
		return false;
	}

	return true;
}

void CCore::Load()
{
	// Check the DirectX version

	U::Signatures.Initialize();
	U::Interfaces.Initialize();

	U::ConVars.Initialize();

	if(!CheckRenderIsCompatible())
		return;

	MH_Initialize();
	DirectX::Startup();
	U::Hooks.Initialize();
	DirectX::bIsReady.store(true);
	
	F::Materials.LoadMaterials();
	F::Commands.Initialize();

	F::Configs.LoadConfig(F::Configs.sCurrentConfig, false);
	F::Menu.ConfigLoaded = true;

	SDK::Output("Amalgam", "Loaded", { 175, 150, 255, 255 });
}

void CCore::Unload()
{
	if(!bUnload)
	{
		G::Unload = true;
		DirectX::bIsReady.store(false);

		U::Hooks.Unload();
		U::ConVars.Unload();
		F::Materials.UnloadMaterials();

		F::Visuals.RestoreWorldModulation();
		Vars::Visuals::World::SkyboxChanger.Value = "Off"; // hooks won't run, remove here
		if(I::Input->CAM_IsThirdPerson())
		{
			auto pLocal = H::Entities.GetLocal();
			if(pLocal)
			{
				I::Input->CAM_ToFirstPerson();
				pLocal->ThirdPersonSwitch();
			}
		}
		if(auto cl_wpn_sway_interp = U::ConVars.FindVar("cl_wpn_sway_interp"))
			cl_wpn_sway_interp->SetValue(0.f);
		if(auto cl_wpn_sway_scale = U::ConVars.FindVar("cl_wpn_sway_scale"))
			cl_wpn_sway_scale->SetValue(0.f);

		Sleep(250);
	}
	SDK::Output("Amalgam", "Unloaded", {175, 150, 255, 255});
}

bool CCore::ShouldUnload()
{
	return SDK::IsGameWindowInFocus() && GetAsyncKeyState(VK_F11) & 0x8000 || bUnload;
}
