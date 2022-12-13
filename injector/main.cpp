#include <iostream>
#include <cstdlib>
#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <windows.h>
#include <string>

void Log(const char* fmt, ...)
{
    SYSTEMTIME	time;
    va_list		ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
}

int unload_dll_by_handle(HANDLE proc, HMODULE dll_handle)
{
	// NOTE: dll_handle is a handle returned by the target process 'proc'.

	// get the address of FreeLibrary()
	HMODULE k32 = GetModuleHandleA("kernel32.dll");
	LPVOID free_library_adr = GetProcAddress(k32, "FreeLibrary");

	HANDLE thread = CreateRemoteThread(proc, NULL, NULL, (LPTHREAD_START_ROUTINE)free_library_adr, dll_handle, NULL, NULL);

	WaitForSingleObject(thread, INFINITE);

	DWORD ret;
	GetExitCodeThread(thread, &ret);

	CloseHandle(thread);

	return ret;
}


int unload_dll_by_name(HANDLE proc, const char* dll)
{
	// Get the dll handle from the target process (GetModuleHandle).
	// The handle is valid only 'inside' the target process.
	// Then use the returned handle to unload the dll (FreeLibrary) in the target process.

	// Write the dll path to target process memory.
	size_t path_len = strlen(dll) + 1;
	LPVOID remote_string_address = VirtualAllocEx(proc, NULL, path_len, MEM_COMMIT, PAGE_EXECUTE);
	WriteProcessMemory(proc, remote_string_address, dll, path_len, NULL);

	HMODULE k32 = GetModuleHandle("kernel32.dll");
	LPVOID get_module_handle_adr = GetProcAddress(k32, "GetModuleHandleA");
	HANDLE thread = CreateRemoteThread(proc, NULL, NULL,
		(LPTHREAD_START_ROUTINE)get_module_handle_adr, remote_string_address, NULL, NULL);
	WaitForSingleObject(thread, INFINITE);

	DWORD dll_handle;
	GetExitCodeThread(thread, &dll_handle);

	CloseHandle(thread);
	VirtualFreeEx(proc, remote_string_address, path_len, MEM_RELEASE);

	if (dll_handle == NULL) {
		return 0;
	}

	return unload_dll_by_handle(proc, (HMODULE)dll_handle);
}

HMODULE load_dll(HANDLE proc, const char* dll_path)
{
	// write the dll path to process memory 
	size_t path_len = strlen(dll_path) + 1;
	LPVOID remote_string_address = VirtualAllocEx(proc, NULL, path_len, MEM_COMMIT, PAGE_EXECUTE);
	WriteProcessMemory(proc, remote_string_address, dll_path, path_len, NULL);

	// get the address of the LoadLibrary()
	HMODULE k32 = GetModuleHandleA("kernel32.dll");
	LPVOID load_library_adr = GetProcAddress(k32, "LoadLibraryA");

	// create the thread
	HANDLE thread = CreateRemoteThread(proc, NULL, NULL, (LPTHREAD_START_ROUTINE)load_library_adr, remote_string_address, NULL, NULL);

	// finish and clean up
	WaitForSingleObject(thread, INFINITE);

	DWORD dll_handle;
	GetExitCodeThread(thread, &dll_handle);

	CloseHandle(thread);

	VirtualFreeEx(proc, remote_string_address, path_len, MEM_RELEASE);

	return (HMODULE)dll_handle;
}

int main()
{
    HANDLE hProcess = 0;
    DWORD processID;

    HWND hwnd = FindWindowA(NULL, "AMAZING ONLINE");
	//HWND hwnd = FindWindowA(NULL, "GTA:SA:MP");

    if (hwnd == NULL)
    {
        Log("Cannot find the game window.");
    }
    else
    {
        Log("I find game window!");
        GetWindowThreadProcessId(hwnd, &processID);
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, processID);

		if (unload_dll_by_name(hProcess, (char*)"G:\\sampr3\\AmazingMinerWH.asi"))
		{
			printf("unload dll!");
		}
		else
		{
			printf("dll not found in memory!");
		}

        if (load_dll(hProcess, (char*)"G:\\sampr3\\AmazingMinerWH.asi"))
        {
            printf("access!");
        }
        else
        {
            printf("fail! %s");
        }
    }
	system("pause");

	unload_dll_by_name(hProcess, (char*)"G:\\sampr3\\AmazingMinerWH.asi");
}