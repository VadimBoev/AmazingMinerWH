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
	HANDLE thread = CreateRemoteThread(proc, NULL, NULL,(LPTHREAD_START_ROUTINE)get_module_handle_adr, remote_string_address, NULL, NULL);
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

std::string GetLastErrorAsString()
{
	//Get the error message ID, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0) {
		return std::string(); //No error message has been recorded
	}

	LPSTR messageBuffer = nullptr;

	//Ask Win32 to give us the string version of that message ID.
	//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	//Copy the error message into a std::string.
	std::string message(messageBuffer, size);

	//Free the Win32's string's buffer.
	LocalFree(messageBuffer);

	return message;
}

#include <filesystem>
#include <iostream>
int main()
{
    HANDLE hProcess = 0;
    DWORD processID;

    //HWND hwnd = FindWindowA(NULL, "AMAZING ONLINE");
	HWND hwnd = FindWindowA(NULL, "GTA:SA:MP");

	std::string path = std::filesystem::current_path().string();
	path += "\\AmazingMinerWH.dll";

	HMODULE test = nullptr;

    if (hwnd == NULL)
    {
        Log("Cannot find the game window.");
    }
    else
    {
        Log("I find game window!");
        GetWindowThreadProcessId(hwnd, &processID);
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, processID);

		while (true)
		{
			if (GetAsyncKeyState(VK_END) & 1)
			{
				int error = unload_dll_by_name(hProcess, "AmazingMinerWH.dll");
				if (error) Log("Success unload from game");
				else Log("Fail unload from game '%s'", GetLastErrorAsString().c_str());
				Log("Error code %d", error);
			}
			if (GetAsyncKeyState(VK_DELETE) & 1)
			{
				test = load_dll(hProcess, (char*)path.c_str());
				if (test) Log("Success load dll!");
				else Log("Fail load dll '%s'", GetLastErrorAsString().c_str());
			}
		}
    }
}