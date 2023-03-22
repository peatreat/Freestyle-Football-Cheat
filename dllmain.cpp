// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>

#include "MinHook/minhook.h"
#pragma comment(lib, "MinHook/libMinHook-x86-v141-mtd.lib")

#include <Psapi.h>

#include "pattern.h"
#include "xor.h"

#define REBASE(pRVA, baseOld, baseNew)       ((uintptr_t)pRVA - (uintptr_t)baseOld + (uintptr_t)baseNew)

uint32_t gameapp_module;
void* unkn_func = nullptr;
void* ball_pos_func = nullptr;

void apply_ball_manipulation(bool enable);

struct v3 {
	float y,z,x;
};

v3* ball_pos = nullptr;
v3 real_ball_pos = {0.f, 15.f, 0.f};
uint8_t old_ball_func_bytes[126];

v3* get_local_pos() {
	if (!gameapp_module)
		return nullptr;

	return (v3*)(*(uint32_t*)(*(uint32_t*)((*(uint32_t*)(*(uint32_t*)(gameapp_module + 0x010198FC) + 0xC)) + 0x18) + 0x410) + 0x80);
}

using fn = int(__thiscall*)(void*, float*, int, void*);
fn oUnkn1 = nullptr;

using pPlayerPos = int(__thiscall*)(void*, void*, void*, int, void*);
pPlayerPos oPlayerPos = nullptr;

bool switch_goals = false;
bool tp_to_ball = false;
bool tp_to_goal = false;

int __fastcall hkUnkn1(void* ecx, void* edx, float* a2, int a3, void* a4)
{
	a2[13] = 50000;
	return oUnkn1(ecx, a2, a3, a4);
}

int __fastcall hkPlayerPos(void* ecx, void* edx, void* a2, void* a3, int a4, void* a5)
{
	if (tp_to_goal || tp_to_ball) {
		v3* local_pos = get_local_pos();

		if (local_pos) {
			if (tp_to_ball) {
				real_ball_pos.y = local_pos->y;
				real_ball_pos.x = local_pos->x;
			}
			else if (tp_to_goal) {
				local_pos->y = (switch_goals) ? (2900.f) : (-2900.f);
				local_pos->x = 0;
				real_ball_pos.y = local_pos->y;
				real_ball_pos.x = local_pos->x;
			}
			apply_ball_manipulation(true);
		}
	}
	else {
		apply_ball_manipulation(false);
	}

	return oPlayerPos(ecx, a2, a3, a4, a5);
}

auto hook = [](void* from, void* to, void** original) {
	MH_Initialize();
	MH_CreateHook(from, to, original);
	MH_EnableHook(from);
};

WNDPROC orgproc = nullptr;
bool enabled = true;
bool shut_down = false;

LRESULT __stdcall proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_KEYDOWN) {
		if (wParam == 0x58) {
			tp_to_ball = true;
		}

		if (wParam == 0x42)
			tp_to_goal = true;
	}

	if (msg == WM_KEYUP) {
		if (wParam == 0x43 && unkn_func) {
			enabled = !enabled;
			if (enabled) {
				MH_EnableHook(unkn_func);
				Beep(523, 50);
			}
			else {
				MH_DisableHook(unkn_func);
				Beep(200, 50);
			}
		}

		if (wParam == 0x58) {
			tp_to_ball = false;
		}

		if (wParam == 0x42)
			tp_to_goal = false;

		if (wParam == 0x4E) {
			switch_goals = !switch_goals;

			if (switch_goals)
				Beep(523, 50);
			else
				Beep(200, 50);
		}

		if (wParam == VK_F6)
			shut_down = true;
	}

	return CallWindowProcA(orgproc, hwnd, msg, wParam, lParam);
}

void apply_ball_manipulation(bool enable) {
	static bool applied = false;
	static uint8_t jmp_pos = 0xBB;
	static uint8_t jmp_pos2 = 0xAA;

	if (enable && !applied) {
		uint8_t new_bytes[] = { 0xA1, 0x00, 0x00, 0x00, 0x00, 0x89, 0x02, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x89, 0x42, 0x04, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x89, 0x42, 0x08, 0x5E, 0x5B, 0x8B, 0xE5, 0x5D, 0xC2, 0x04, 0x00, 0x8B, 0x4D, 0x08, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x89, 0x01, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x89, 0x41, 0x04, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x89, 0x41, 0x08, 0x5F, 0x5E, 0x5B, 0x8B, 0xE5, 0x5D, 0xC2, 0x04, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

		*(uint32_t*)(new_bytes + 0x1) = (uint32_t)&real_ball_pos.y;
		*(uint32_t*)(new_bytes + 0x23) = (uint32_t)&real_ball_pos.y;
		*(uint32_t*)(new_bytes + 0x8) = (uint32_t)&real_ball_pos.z;
		*(uint32_t*)(new_bytes + 0x2A) = (uint32_t)&real_ball_pos.z;
		*(uint32_t*)(new_bytes + 0x10) = (uint32_t)&real_ball_pos.x;
		*(uint32_t*)(new_bytes + 0x32) = (uint32_t)&real_ball_pos.x;

		if (ball_pos_func) {
			void* ball_still_addr = (LPBYTE)ball_pos_func + 0x1F;

			jmp_pos = *(uint8_t*)((LPBYTE)ball_pos_func - 0x63 + 0x2);
			jmp_pos2 = *(uint8_t*)((LPBYTE)ball_pos_func - 0x52 + 0x2);

			memcpy(old_ball_func_bytes, ball_pos_func, sizeof(old_ball_func_bytes));

			DWORD old;
			VirtualProtect(ball_pos_func, sizeof(new_bytes), PAGE_READWRITE, &old);

			memcpy(ball_pos_func, new_bytes, sizeof(new_bytes));

			*(uint8_t*)((LPBYTE)ball_pos_func - 0x63 + 0x2) = (LPBYTE)ball_still_addr - ((LPBYTE)ball_pos_func - 0x63 + 0x6);
			*(uint8_t*)((LPBYTE)ball_pos_func - 0x52 + 0x2) = (LPBYTE)ball_still_addr - ((LPBYTE)ball_pos_func - 0x52 + 0x6);

			VirtualProtect(ball_pos_func, sizeof(new_bytes), old, &old);

			applied = true;
		}
	}
	else if (!enable && applied) {
		DWORD old;
		VirtualProtect(ball_pos_func, sizeof(old_ball_func_bytes), PAGE_READWRITE, &old);

		*(uint8_t*)((LPBYTE)ball_pos_func - 0x63 + 0x2) = jmp_pos;
		*(uint8_t*)((LPBYTE)ball_pos_func - 0x52 + 0x2) = jmp_pos2;

		memcpy(ball_pos_func, old_ball_func_bytes, sizeof(old_ball_func_bytes));

		VirtualProtect(ball_pos_func, sizeof(old_ball_func_bytes), old, &old);

		applied = false;
	}
}

void init() {
	auto fsf = FindWindowW(0, _(L"FreestyleFootball"));
	while (!fsf) {
		fsf = FindWindowW(0, _(L"FreestyleFootball"));
		Sleep(10);
	}

	gameapp_module = reinterpret_cast<std::uintptr_t>(GetModuleHandleA(_("FSeFootball.exe")));

	ball_pos = pattern::search(_("FSeFootball.exe"), _("8B C3 89 0D")).add(0x4).deref().sub(0x8).get<v3*>();
	ball_pos_func = pattern::search(_("FSeFootball.exe"), _("F3 0F 11 52 ?? F3 0F 10")).get<void*>();

	orgproc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(fsf, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(proc)));

	auto player_pos_func = pattern::search(_("FSeFootball.exe"), _("55 8B EC 83 EC 7C A1 ? ? ? ? 33 C5 89 45 FC 8B 45 0C 53 8B 5D 08")).get<void*>();
	hook(player_pos_func, &hkPlayerPos, (void**)&oPlayerPos);

	unkn_func = pattern::search(_("FSeFootball.exe"), _("55 8B EC 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 F8 8B 45 10 8B D1 53 56")).get<void*>();
	hook(unkn_func, &hkUnkn1, (void**)&oUnkn1);

	while (!shut_down) Sleep(100);

	SetWindowLongPtrW(fsf, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(orgproc));

	apply_ball_manipulation(false);

	MH_DisableHook(player_pos_func);
	MH_RemoveHook(player_pos_func);

	MH_DisableHook(unkn_func);
	MH_RemoveHook(unkn_func);

	MH_Uninitialize();

	Beep(200, 200);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
	case DLL_PROCESS_ATTACH: {
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)init, 0, 0, nullptr);
		//MessageBoxA(0, "hi im injected", 0, 0);
		break;
	}
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

