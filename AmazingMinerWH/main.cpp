#include "main.h"

uintptr_t SampDLL = 0;
uintptr_t AmazingDLL = 0;

#define inRPC 0
#define outRPC 1
#define inPacket 2
#define outPacket 3

FILE* g_flLog = NULL;
void Log(const char* fmt, ...)
{
	SYSTEMTIME	time;
	va_list		ap;

	//write in file
	if (g_flLog == NULL)
	{
		char	filename[512];
		snprintf(filename, sizeof(filename), "G:\\sampr3\\client.log");

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

void RenderDebug();

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

		//mouse fix
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

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
				//RenderDebug();

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

uintptr_t GetOffset(uintptr_t addr)
{
	return addr - 0x541C1000;
}

std::list<std::string> InRPC;
std::list<std::string> OutRPC;
std::list<std::string> InPacket;
std::list<std::string> OutPacket;
void RenderDebug()
{
	ImGui::SetNextWindowSize(ImVec2(950,350));
	ImGui::Begin("Amazing Debug");

	ImGui::Text("Debug InRPC");
	for (auto entry : InRPC)
	{
		ImGui::Text("%s", entry.c_str());
	}

	ImGui::Text("\n");

	ImGui::Text("Debug OutRPC");
	for (auto entry : OutRPC)
	{
		ImGui::Text("%s", entry.c_str());
	}

	ImGui::Text("AmazingDLL 0x%X", AmazingDLL);
	//ImGui::Text("Amazing test 0x%X", GetOffset());

	/*
	ImGui::Text("\n");

	ImGui::Text("Debug InPacket");
	for (auto entry : InPacket)
	{
		ImGui::Text("%s", entry.c_str());
	}

	ImGui::Text("\n");

	ImGui::Text("Debug OutPacket");
	for (auto entry : OutPacket)
	{
		ImGui::Text("%s", entry.c_str());
	}
	*/
	ImGui::End();
}

void DebugWrite(BYTE type, char* szFormat, ...)
{
	char tmp_buf[512];
	memset(tmp_buf, 0, sizeof(tmp_buf));

	va_list args;
	va_start(args, szFormat);
	vsprintf(tmp_buf, szFormat, args);
	va_end(args);

	if (type == inRPC)
	{
		if (InRPC.size() >= 8)
			InRPC.pop_front();

		InRPC.push_back(tmp_buf);
	}
	else if (type == outRPC)
	{
		if (OutRPC.size() >= 8)
			OutRPC.pop_front();

		OutRPC.push_back(tmp_buf);
	}
	else if (type == inPacket)
	{
		if (InPacket.size() >= 8)
			InPacket.pop_front();

		InPacket.push_back(tmp_buf);
	}
	else if (type == outPacket)
	{
		if (OutPacket.size() >= 8)
			OutPacket.pop_front();

		OutPacket.push_back(tmp_buf);
	}
}

bool (__thiscall* oOutRPC)(void*, int*, BitStream*, PacketPriority, PacketReliability, char, bool);
bool __fastcall hkOutRPC(void* pThis, void* pUnk, int* uniqueID, BitStream* bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, bool shiftTimestamp)
{
	if (*uniqueID == 252)
	{
		int iBitLength = bitStream->GetNumberOfBitsUsed();

		if (iBitLength == 432)
		{
			BitStream bsData((unsigned char*)bitStream->GetData(), (iBitLength / 8) + 1, false);

			BYTE unk1;
			WORD unk2;
			BYTE unk3;
			BYTE unk4;
			BYTE unk5;

			bsData.Read(unk1);
			bsData.Read(unk2);
			bsData.Read(unk3);
			bsData.Read(unk4);
			bsData.Read(unk5);

			DebugWrite(outRPC, "%d, %d, %d, %d, %d", unk1, unk2, unk3, unk4, unk5);
		}
	}

	return oOutRPC(pThis, uniqueID, bitStream, priority, reliability, orderingChannel, shiftTimestamp);
}

#define MAX_ALLOCA_STACK_ALLOCATION 1048576

bool RPC(int uniqueID, BitStream* bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, bool shiftTimestamp)
{
	return ((bool (*)(RakClientInterface*, int, BitStream*, PacketPriority, PacketReliability, char, bool))(SampDLL + 0x33EE0))(sampapi::v037r3::RefNetGame()->m_pRakClient, uniqueID, bitStream, priority, reliability, orderingChannel, shiftTimestamp);
}

bool (__thiscall* oInRPC)(void*, const char*, int, PlayerID);
bool __fastcall hkInRPC(void* thiz, void* b, const char* data, int length, PlayerID playerId)
{
	BitStream incomingBitStream((unsigned char*)data, length, false);
	int* uniqueIdentifier = 0;
	unsigned char* userData;
	RPCNode* node;
	RPCParameters rpcParms;
	BitStream replyToSender;
	rpcParms.replyToSender = &replyToSender;

	rpcParms.recipient = 0;
	rpcParms.sender = playerId;

	// Note to self - if I change this format then I have to change the PacketLogger class too
	incomingBitStream.IgnoreBits(8);
	if (data[0] == ID_TIMESTAMP)
		incomingBitStream.IgnoreBits(8 * (sizeof(RakNetTime) + sizeof(unsigned char)));

	incomingBitStream.Read((char*)&uniqueIdentifier, 1);
	if (incomingBitStream.ReadCompressed(rpcParms.numberOfBitsOfData) == false)
	{
		return false;
	}

	// Call the function
	if (rpcParms.numberOfBitsOfData == 0)
	{

	}
	else
	{
		if (incomingBitStream.GetNumberOfUnreadBits() == 0)
		{
			return false; // No data was appended!
		}

		// We have to copy into a new data chunk because the user data might not be byte aligned.
		bool usedAlloca = false;

		if (BITS_TO_BYTES(incomingBitStream.GetNumberOfUnreadBits()) < MAX_ALLOCA_STACK_ALLOCATION)
		{
			userData = (unsigned char*)alloca(BITS_TO_BYTES(incomingBitStream.GetNumberOfUnreadBits()));
			usedAlloca = true;
		}
		else
			userData = new unsigned char[BITS_TO_BYTES(incomingBitStream.GetNumberOfUnreadBits())];

		if (incomingBitStream.ReadBits((unsigned char*)userData, rpcParms.numberOfBitsOfData, false) == false)
		{
			return false; // Not enough data to read
		}

		// Call the function callback
		rpcParms.input = userData;

		if(uniqueIdentifier == (int*)252)
		{
			unsigned char* Data = reinterpret_cast<unsigned char*>(rpcParms.input);
			int iBitLength = rpcParms.numberOfBitsOfData;

			//if 41 bitlength == show or hide button ???

		

			//if (iBitLength == 472 || iBitLength == 41)
			//{
				BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);
				 
				BYTE unk1;
				WORD unk2;
				BYTE unk3;
				BYTE unk4;
				BYTE unk5;
				BYTE unk6;
				BYTE unk7;
				BYTE unk8;
				BYTE unk9;
				BYTE unk10;
				BYTE unk11;

				bsData.Read(unk1);
				bsData.Read(unk2);
				bsData.Read(unk3);
				bsData.Read(unk4);
				bsData.Read(unk5);
				bsData.Read(unk6);
				bsData.Read(unk7);
				bsData.Read(unk8);
				bsData.Read(unk9);
				bsData.Read(unk10);
				bsData.Read(unk11);

				DebugWrite(inRPC, "[%d] id: %d (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)", iBitLength, uniqueIdentifier, unk1, unk2, unk3, unk4, unk5, unk6, unk7, unk8, unk9, unk10, unk11);
				
			//}

				//RPC(252, &bsData, HIGH_PRIORITY, RELIABLE_ORDERED, 7, false);
		}


		if (usedAlloca == false)
			delete[] userData;
	}

	return oInRPC(thiz, data, length, playerId);
}

Packet* (__fastcall* oInPacket)(void*);
Packet* __fastcall hkInPacket(void* dis)
{
	Packet* packet = oInPacket(dis);
	if (packet != nullptr && packet->data && packet->length > 0) 
	{
		DebugWrite(inPacket, "id %d", packet->data[0]);
	}
	return packet;
}

void (__fastcall* o_bsWrite)(int* thiz, unsigned int* a2, signed int a3, char a4);
void __fastcall hk_bsWrite(int* thiz, unsigned int* a2, signed int a3, char a4)
{
	DebugWrite(inRPC, "a3: %d", a3);
	return o_bsWrite(thiz, a2, a3, a4);
}

DWORD WINAPI InitializeAndLoad(LPVOID hModule)
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

	AmazingDLL = reinterpret_cast<uintptr_t>(GetModuleHandle("aggmain.asi"));
	Log("***Base aggmain.as address = 0x%X", AmazingDLL);

	MH_CreateHook(get_function_address(17), &IDirect3DDevice9__Present_Hook, reinterpret_cast<void**>(&IDirect3DDevice9__Present));
	MH_EnableHook(get_function_address(17));

	MH_CreateHook(get_function_address(16), &IDirect3DDevice9__Reset_Hook, reinterpret_cast<void**>(&IDirect3DDevice9__Reset));
	MH_EnableHook(get_function_address(16));

	//out rpc
	MH_CreateHook((void*)(SampDLL + 0x33EE0), &hkOutRPC, (void**)(&oOutRPC));
	MH_EnableHook((void*)(SampDLL + 0x33EE0));

	//HandleRPCPacket incoming rpc
	MH_CreateHook((void*)(SampDLL + 0x3A6A0), &hkInRPC, (void**)(&oInRPC));
	MH_EnableHook((void*)(SampDLL + 0x3A6A0));

	//MH_CreateHook((void*)(SampDLL + 0x1F8A0), &hk_bsWrite, (void**)(&o_bsWrite));
	//MH_EnableHook((void*)(SampDLL + 0x1F8A0));

	//CRakPeer::ReceivePacket (incoming packet)
	//R1 0x31710 R3 0x34530
	//MH_CreateHook((void*)(SampDLL + 0x34530), &hkInPacket, (void**)(&oInPacket));
	//MH_EnableHook((void*)(SampDLL + 0x34530));

	//CRakPeer::Send (outcoming packet)
	//R1 0x307F0 R3 0x33BA0
	//MH_CreateHook((void*)(SampDLL + 0x33BA0), &hkOutPacket, (void**)(&oOutPacket));
	//MH_EnableHook((void*)(SampDLL + 0x33BA0));

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

			CreateThread(0, 0, &InitializeAndLoad, hModule, 0, 0);

			break;
		}
		case DLL_PROCESS_DETACH:
		{
			MH_DisableHook(get_function_address(17));
			MH_DisableHook(get_function_address(16));
			MH_DisableHook((void*)(SampDLL + 0x33EE0)); //hook RPC
			MH_DisableHook((void*)(SampDLL + 0x3A6A0)); //hook HandleRPCPacket
			//MH_DisableHook((void*)(SampDLL + 0x34530)); //hook CRakPeer::ReceivePacket

			MH_RemoveHook(get_function_address(17));
			MH_RemoveHook(get_function_address(16));
			MH_RemoveHook((void*)(SampDLL + 0x33EE0)); //hook RPC
			MH_RemoveHook((void*)(SampDLL + 0x3A6A0)); //hook HandleRPCPacket
			//MH_RemoveHook((void*)(SampDLL + 0x34530)); //hook CRakPeer::ReceivePacket

			///SetWindowLongPtr(**(HWND**)0xC17054, GWL_WNDPROC, (LRESULT)oWndProc);
			SetWindowLongA(FindWindowA(NULL, "AMAZING ONLINE"), GWL_WNDPROC, LONG(oWndProc));

			break;
		}
	}
	return 1;
}