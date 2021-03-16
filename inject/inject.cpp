#include <span>
#include <iostream>
#include <string_view>
#include <filesystem>

#include <Windows.h>
#include <Psapi.h>

namespace fs = std::filesystem;

int wmain(int argc, wchar_t** argv)
{
	auto const args = std::span(argv, argc);
	if (args.size() < 3)
	{
		auto const binary_name = fs::path{ args[0] }.filename();
		std::wcerr << "usage: " << binary_name << " EXE DLLs..." << std::endl;
		return 1;
	}

	auto const exe = args[1];

	auto si = STARTUPINFO{ .cb = sizeof(STARTUPINFO) };
	auto pi = PROCESS_INFORMATION{};
	if (!CreateProcess(NULL, exe, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi))
	{
		std::wcerr << "CreateProcess failed" << std::endl;
		return 1;
	}

	auto const page = VirtualAllocEx(pi.hProcess, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (page == nullptr)
	{
		std::wcerr << "VirtualAllocEx failed" << std::endl;
		return 1;
	}

	auto ret = 0;
	for (std::wstring_view dll : args.subspan(2))
	{
		if (dll.size() >= MAX_PATH)
		{
			std::wcerr << "dll path length exceeds MAX_PATH" << std::endl;
			ret = 1;
			break;
		}

		if (GetFileAttributes(dll.data()) == INVALID_FILE_ATTRIBUTES)
		{
			std::wcerr << "unable to locate library: " << dll << std::endl;
			ret = 1;
			break;
		}

		if (WriteProcessMemory(pi.hProcess, page, dll.data(), (dll.size() + 1) * sizeof(wchar_t), NULL) == 0)
		{
			std::wcerr << "WriteProcessMemory failed" << std::endl;
			ret = 1;
			break;
		}

		auto hThread = CreateRemoteThread(pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, page, 0, NULL);
		if (hThread == nullptr)
		{
			std::wcerr << "CreateRemoteThread failed" << std::endl;
			ret = 1;
			break;
		}

		if (WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED)
		{
			std::wcerr << "WaitForSingleObject failed" << std::endl;
			ret = 1;
			break;
		}

		CloseHandle(hThread);
	}

	if (ResumeThread(pi.hThread) == -1)
	{
		std::wcerr << "ResumeThread failed" << std::endl;
		return 1;
	}

	CloseHandle(pi.hProcess);
	VirtualFreeEx(pi.hProcess, page, 0, MEM_RELEASE);

	return ret;
}