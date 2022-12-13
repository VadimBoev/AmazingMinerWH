#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cstdlib>
#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <windows.h>
#include <string>

#include <fcntl.h>
#include <io.h>

#include <d3dx9.h>
#include "MinHook.h"
#pragma comment(lib, "libMinHook.x86.lib")

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_internal.h"


#include "CObjectPool.h"
#include "CNetGame.h"
#include "CGame.h"

#define SAMP_037_R3_1

uintptr_t SampDLL = 0;

FILE* g_flLog = NULL;
void Log(const char* fmt, ...)
{
	SYSTEMTIME	time;
	va_list		ap;

	//write in file
	if (g_flLog == NULL)
	{
		char	filename[512];
		snprintf(filename, sizeof(filename), "C:\\Users\\VadimPC\\source\\repos\\AmazingMinerWH\\Debug\\client.log");

		g_flLog = fopen(filename, "w");
		if (g_flLog == NULL)
			return;
	}

	GetLocalTime(&time);
	fprintf(g_flLog, "[%02d:%02d:%02d.%03d] ", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
	va_start(ap, fmt);
	vfprintf(g_flLog, fmt, ap);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
	fprintf(g_flLog, "\n");
	fflush(g_flLog);
}

void UnFuck(DWORD addr, int size)
{
	DWORD d;
	VirtualProtect((PVOID)addr, size, PAGE_EXECUTE_READWRITE, &d);
}

bool CreateConsole()
{
	int hConHandle = 0;
	HANDLE lStdHandle = 0;
	FILE* fp = 0;
	AllocConsole();
	freopen("CON", "w", stdout);
	SetConsoleTitle("Amazing Online Debug");
	HWND hwnd = ::GetConsoleWindow();
	if (hwnd != NULL)
	{
		HMENU hMenu = ::GetSystemMenu(hwnd, FALSE);
		if (hMenu != NULL)
		{
			//DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
			DeleteMenu(hMenu, SC_MINIMIZE, MF_BYCOMMAND);
			DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);

			SetWindowPos(hwnd, 0, -900, 100, 800, 600, 0);
		}
	}
	lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle(PtrToUlong(lStdHandle), _O_TEXT);
	fp = _fdopen(hConHandle, "w");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);

	return true;
}

std::uintptr_t find_device(std::uint32_t Len)
{
	static std::uintptr_t base = [](std::size_t Len)
	{
		std::string path_to(MAX_PATH, '\0');
		if (auto size = GetSystemDirectoryA((LPSTR)path_to.data(), MAX_PATH))
		{
			path_to.resize(size);
			path_to += "\\d3d9.dll";
			std::uintptr_t dwObjBase = reinterpret_cast<std::uintptr_t>(LoadLibraryA(path_to.c_str()));
			while (dwObjBase++ < dwObjBase + Len)
			{
				if (*reinterpret_cast<std::uint16_t*>(dwObjBase + 0x00) == 0x06C7 &&
					*reinterpret_cast<std::uint16_t*>(dwObjBase + 0x06) == 0x8689 &&
					*reinterpret_cast<std::uint16_t*>(dwObjBase + 0x0C) == 0x8689)
				{
					dwObjBase += 2;
					break;
				}
			}
			return dwObjBase;
		}
		return std::uintptr_t(0);
	}(Len);
	return base;
}

void* get_function_address(int VTableIndex)
{
	return (*reinterpret_cast<void***>(find_device(0x128000)))[VTableIndex];
}

WNDPROC oWndProc;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

	if (ImGui::GetIO().WantTextInput)
	{
		((void (*)())(0x53F1E0))(); // CPad::ClearKeyboardHistory
		return 1;
	}

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}


D3DXVECTOR2 CalcScreenCoords(D3DXVECTOR3 vecWorld)
{
	float x, y, z;
	const D3DXMATRIX m{ reinterpret_cast<float*>(0xB6FA2C) };
	unsigned long dwLenX = *reinterpret_cast<unsigned long*>(0xC17044);
	unsigned long dwLenY = *reinterpret_cast<unsigned long*>(0xC17048);

	x = (vecWorld.z * m._31) + (vecWorld.y * m._21) + (vecWorld.x * m._11) + m._41;
	y = (vecWorld.z * m._32) + (vecWorld.y * m._22) + (vecWorld.x * m._12) + m._42;
	z = (vecWorld.z * m._33) + (vecWorld.y * m._23) + (vecWorld.x * m._13) + m._43;

	double    fRecip = (double)1.0 / z;
	x *= (float)(fRecip * dwLenX);
	y *= (float)(fRecip * dwLenY);
	return { x, y };
}

bool IsObjOnScreen(sampapi::v037r3::CObject* object)
{
	float x, y, z;

	const D3DXMATRIX m{ reinterpret_cast<float*>(0xB6FA2C) };
	unsigned long dwLenX = *reinterpret_cast<unsigned long*>(0xC17044);
	unsigned long dwLenY = *reinterpret_cast<unsigned long*>(0xC17048);

	x = object->m_position.x;
	y = object->m_position.y;
	z = object->m_position.z;
	D3DXVECTOR2 screenPos = CalcScreenCoords({ x, y, z });

	z = (z * m._33) + (y * m._23) + (x * m._13) + m._43;
	if (screenPos.x <= dwLenX && screenPos.y <= dwLenY && screenPos.x >= 0 && screenPos.y >= 0 && z >= 1.0f)
		return true;
	return false;
}

D3DXVECTOR3 GetObjectCoordinates(sampapi::v037r3::CObject* object) 
{
	float x, y, z;
	x = object->m_position.x;
	y = object->m_position.y;
	z = object->m_position.z;
	return { x, y, z };
}

D3DXVECTOR3 GetPedCoordinates() 
{
	float x, y, z;
	DWORD CPed = *reinterpret_cast<DWORD*>(0xB6F5F0);
	if (CPed != NULL) {
		DWORD CPed_stPos = *reinterpret_cast<DWORD*>(CPed + 0x14);
		x = *reinterpret_cast<float*>(CPed_stPos + 0x30);
		y = *reinterpret_cast<float*>(CPed_stPos + 0x34);
		z = *reinterpret_cast<float*>(CPed_stPos + 0x38);
		return { x, y, z };
	}
	return { 0.0f, 0.0f, 0.0f };
}


int objectId[4] = { 7234, 7235, 7236, 7237 };
const char* objectName[4] = { "coal", "iron", "gold", "DIAMONDS" };

HRESULT(__stdcall* IDirect3DDevice9__Present)(LPDIRECT3DDEVICE9 pDevice, CONST RECT* pSrcRect, CONST RECT* pDestRect, HWND hDestWindow, CONST RGNDATA* pDirtyRegion);
HRESULT __stdcall IDirect3DDevice9__Present_Hook(LPDIRECT3DDEVICE9 pDevice, CONST RECT* pSrcRect, CONST RECT* pDestRect, HWND hDestWindow, CONST RGNDATA* pDirtyRegion)
{
	static bool ImInit = false;

	if (!ImInit)
	{
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(**(HWND**)0xC17054);

		oWndProc = (WNDPROC)SetWindowLongPtr(**(HWND**)0xC17054, GWL_WNDPROC, (LRESULT)WndProc);

		//disable file settings 'imgui.ini'
		ImGui::GetIO().IniFilename = NULL;

		ImGui_ImplWin32_EnableDpiAwareness();

		//init DX9 
		ImGui_ImplDX9_Init(pDevice);

		//set style imgui (default)
		ImGui::StyleColorsDark();

		ImInit = true;
	}
	else
	{
		//ImGui::GetIO().DeltaTime = *(float*)0xB7CB5C / 50.f;

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		if (SampDLL != 0)
		{
			if (sampapi::v037r3::RefNetGame())
			{
				auto drawlist = ImGui::GetBackgroundDrawList();

				for (int i = 0; i < 1000; i++)
				{
					sampapi::v037r3::CObjectPool* pool = sampapi::v037r3::RefNetGame()->GetObjectPool();
					if (pool->m_pObject[i] != nullptr && pool->Get(i) != nullptr)
					{
						int model = pool->Get(i)->m_nModel;
						D3DXVECTOR3 pedPos = GetPedCoordinates();
						D3DXVECTOR2 pedScreenPos = CalcScreenCoords(pedPos);
						for (int b = 0; b < 4; b++)
						{
							if (model == objectId[b]) 
							{
								if (IsObjOnScreen(pool->Get(i)))
								{
									D3DXVECTOR3 objPos = GetObjectCoordinates(pool->Get(i));
									D3DXVECTOR2 objScreenPos = CalcScreenCoords(objPos);

									if(b == 0)
										drawlist->AddText(ImVec2(objScreenPos.x, objScreenPos.y), ImColor(157, 52, 44), objectName[b]);
									else if(b == 1)
										drawlist->AddText(ImVec2(objScreenPos.x, objScreenPos.y), ImColor(151, 126, 14), objectName[b]);
									else if(b == 2)
										drawlist->AddText(ImVec2(objScreenPos.x, objScreenPos.y), ImColor(255, 208, 0), objectName[b]);
									else if(b == 3)
										drawlist->AddText(ImVec2(objScreenPos.x, objScreenPos.y), ImColor(0, 255, 255), objectName[b]);
								}
							}
						}
					}
				}
			}
		}
		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	}

	return IDirect3DDevice9__Present(pDevice, pSrcRect, pDestRect, hDestWindow, pDirtyRegion);
}

HRESULT(__stdcall* IDirect3DDevice9__Reset)(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
HRESULT __stdcall IDirect3DDevice9__Reset_Hook(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	return IDirect3DDevice9__Reset(pDevice, pPresentationParameters);
}

DWORD WINAPI InitializeAndLoad(LPVOID)
{
	Log("InitializeAndLoad");

	while (SampDLL == NULL)
	{
		SampDLL = reinterpret_cast<uintptr_t>(GetModuleHandle("azmp.dll"));
		//SampDLL = reinterpret_cast<uintptr_t>(GetModuleHandle("samp.dll"));
		Sleep(1);
	}

	if (*(DWORD*)(SampDLL + 0x6FF4D) == 0xA164) //R3
	{
		Log("***Base AZ:MP (SA:MP 0.3.7 R3) address = 0x%X", SampDLL);
	}

	MH_CreateHook(get_function_address(17), &IDirect3DDevice9__Present_Hook, reinterpret_cast<void**>(&IDirect3DDevice9__Present));
	MH_EnableHook(get_function_address(17));

	MH_CreateHook(get_function_address(16), &IDirect3DDevice9__Reset_Hook, reinterpret_cast<void**>(&IDirect3DDevice9__Reset));
	MH_EnableHook(get_function_address(16));

	//CreateThread(0, 0, &GetKeys, 0, 0, 0);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
			//CreateConsole();
			Log("AMAZING ONLINE injected!");
			DisableThreadLibraryCalls(hModule);

			MH_Initialize();

			CreateThread(0, 0, &InitializeAndLoad, 0, 0, 0);

			break;
		}
		case DLL_PROCESS_DETACH:
		{
			break;
		}
	}
	return 1;
}