#include "GetCmdLine.h"
#include <stdio.h>
#include <tchar.h>
#include <sstream>

Logging::InsertDate format;

PWSTR GetCMDLine(DWORD pid, Logging::AbstractLogger *_log)
{
	// get process identifier
	DWORD dwId = pid;
	//= 5616L;

	// open the process
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwId);
	DWORD err = 0;
	if (hProcess == NULL)
	{
		std::ostringstream s;
		err = GetLastError();
		s << "OpenProcess " << dwId << " failed with error " << err;
		_log->LogMessage(Logging::Sever, format, s.str());
		throw MsgException("FAIL");
	}

	// determine if 64 or 32-bit processor
	SYSTEM_INFO si;
	GetNativeSystemInfo(&si);

	// determine if this process is running on WOW64
	BOOL wow;
	IsWow64Process(GetCurrentProcess(), &wow);

	// use WinDbg "dt ntdll!_PEB" command and search for ProcessParameters offset to find the truth out
	DWORD ProcessParametersOffset = si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? 0x20 : 0x10;
	DWORD CommandLineOffset = si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? 0x70 : 0x40;

	// read basic info to get ProcessParameters address, we only need the beginning of PEB
	DWORD pebSize = ProcessParametersOffset + 8;
	PBYTE peb = (PBYTE)malloc(pebSize);
	ZeroMemory(peb, pebSize);

	// read basic info to get CommandLine address, we only need the beginning of ProcessParameters
	DWORD ppSize = CommandLineOffset + 16;
	PBYTE pp = (PBYTE)malloc(ppSize);
	ZeroMemory(pp, ppSize);

	PWSTR cmdLine;

	if (wow)
	{
		// we're running as a 32-bit process in a 64-bit OS
		PROCESS_BASIC_INFORMATION_WOW64 pbi;
		ZeroMemory(&pbi, sizeof(pbi));

		// get process information from 64-bit world
		_NtQueryInformationProcess query = (_NtQueryInformationProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtWow64QueryInformationProcess64");
		err = query(hProcess, 0, &pbi, sizeof(pbi), NULL);
		if (err != 0)
		{
			_log->LogMessage(Logging::Sever, format, "NtWow64QueryInformationProcess64 failed");
			CloseHandle(hProcess);
			throw MsgException("FAIL");
		}

		// read PEB from 64-bit address space
		_NtWow64ReadVirtualMemory64 read = (_NtWow64ReadVirtualMemory64)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtWow64ReadVirtualMemory64");
		err = read(hProcess, pbi.PebBaseAddress, peb, pebSize, NULL);
		if (err != 0)
		{
			_log->LogMessage(Logging::Sever, format, "NtWow64ReadVirtualMemory64 PEB failed");
			CloseHandle(hProcess);
			throw MsgException("FAIL");
		}

		// read ProcessParameters from 64-bit address space
		PBYTE* parameters = (PBYTE*)*(LPVOID*)(peb + ProcessParametersOffset); // address in remote process adress space
		err = read(hProcess, parameters, pp, ppSize, NULL);
		if (err != 0)
		{
			_log->LogMessage(Logging::Sever, format, "NtWow64ReadVirtualMemory64 Parameters failed");
			CloseHandle(hProcess);
			throw MsgException("FAIL");
		}

		// read CommandLine
		UNICODE_STRING_WOW64* pCommandLine = (UNICODE_STRING_WOW64*)(pp + CommandLineOffset);
		cmdLine = (PWSTR)malloc(pCommandLine->MaximumLength);
		err = read(hProcess, pCommandLine->Buffer, cmdLine, pCommandLine->MaximumLength, NULL);
		if (err != 0)
		{
			_log->LogMessage(Logging::Sever, format, "NtWow64ReadVirtualMemory64 Parameters failed");
			CloseHandle(hProcess);
			throw MsgException("FAIL");
		}
	}
	else
	{
		// we're running as a 32-bit process in a 32-bit OS, or as a 64-bit process in a 64-bit OS
		PROCESS_BASIC_INFORMATION pbi;
		ZeroMemory(&pbi, sizeof(pbi));

		// get process information
		_NtQueryInformationProcess query = (_NtQueryInformationProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
		err = query(hProcess, 0, &pbi, sizeof(pbi), NULL);
		if (err != 0)
		{
			_log->LogMessage(Logging::Sever, format, "NtQueryInformationProcess failed");
			CloseHandle(hProcess);
			throw MsgException("FAIL");
		}

		// read PEB
		if (!ReadProcessMemory(hProcess, pbi.PebBaseAddress, peb, pebSize, NULL))
		{
			_log->LogMessage(Logging::Sever, format, "ReadProcessMemory PEB failed");
			CloseHandle(hProcess);
			throw MsgException("FAIL");
		}

		// read ProcessParameters
		PBYTE* parameters = (PBYTE*)*(LPVOID*)(peb + ProcessParametersOffset); // address in remote process adress space
		if (!ReadProcessMemory(hProcess, parameters, pp, ppSize, NULL))
		{
			_log->LogMessage(Logging::Sever, format, "ReadProcessMemory Parameters failed");
			CloseHandle(hProcess);
			throw MsgException("FAIL");
		}

		// read CommandLine
		UNICODE_STRING* pCommandLine = (UNICODE_STRING*)(pp + CommandLineOffset);
		cmdLine = (PWSTR)malloc(pCommandLine->MaximumLength);
		if (!ReadProcessMemory(hProcess, pCommandLine->Buffer, cmdLine, pCommandLine->MaximumLength, NULL))
		{
			_log->LogMessage(Logging::Sever, format, "ReadProcessMemory Parameters failed");
			CloseHandle(hProcess);
			throw MsgException("FAIL");
		}
	}
	//printf("%S\n", cmdLine);
	return cmdLine;
}