#include "../SDK/SDK.h"

#include "../Features/Misc/Misc.h"

MAKE_HOOK(BaseClientDLL_Shutdown, U::Memory.GetVFunc(I::BaseClientDLL, 7), void, __fastcall,
	void* ecx)
{
	H::Entities.Clear();
	G::DormancyMap.clear();
	G::ChokeMap.clear();
	G::IsConnected = false;

	CALL_ORIGINAL(ecx);
}

MAKE_HOOK(BaseClientDLL_LevelInitPreEntity, U::Memory.GetVFunc(I::BaseClientDLL, 5), void, __fastcall, 
	void* ecx, const char* pMapName)
{
	G::IsConnected = true;

	CALL_ORIGINAL(ecx, pMapName);
}