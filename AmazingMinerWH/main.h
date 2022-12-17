#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cstdlib>
#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <windows.h>
#include <string>
#include <list>

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

#include "BitStream.h"
#include "RakClient.h"

#define SAMP_037_R3_1