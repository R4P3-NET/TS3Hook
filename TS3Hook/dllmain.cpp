﻿// dllmain.cpp : Defines the entry point for the DLL application.
#include "main.h"
#include <cstdio>
#include "PatchTools.h"
#include <iostream>
#include <string>

#ifdef ENV32
const LPCWSTR mod = L"ts3client_win32.exe";

// Ver: 3.1.6>3.1.4.2>3.0.17  !3.0.16
const char* MASK_IN_1 = "\x8B\x4F\x3C\x6A\x00\xFF\x77\x44\xFF\x77\x40\x8B\x01\x57\x56\xFF\x50\x10";
const char* PATT_IN_1 = "xxxxxxxxxxxxxxxxxx";

// Ver: 3.1.6>3.1.4.2>3.1>?  !3.0.17
const char* MASK_OUT_1 = "\xC6\x45\xFC\x06\x80\xF9\x02\x74\x09\x80\xF9\x03";
const char* PATT_OUT_1 = "xxxxxxxxxxxx";
#else
const LPCWSTR mod = L"ts3client_win64.exe";

const char* MASK_IN_1 = "\x49\x8B\x4E\x50\x48\x8B\x01\xC6\x44\x24\x20\x00\x4D\x8B\x4E\x58\x4D\x8B\xC6\x48\x8B\xD3\xFF\x50\x20\xEB";
const char* PATT_IN_1 = "xxxxxxxxxxxxxxxxxxxxxxxxxx";

const char* MASK_OUT_1 = "\x89\x45\x00\x83\xF8\x01\x0F\x94\xC1\x88\x4C\x24\x44\x80\x7C\x24\x40\x00";
const char* PATT_OUT_1 = "xxxxxxxxxxxxxxxxxx";
#endif

// RUNTIME CALCED
extern "C"
{
	SIZE_T packet_in_hook_return = 0x0;
	SIZE_T packet_out_hook_return = 0x0;
}

BOOL APIENTRY DllMain(HMODULE hModule, const DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:

		printf("-==== TS3HOOK 1.0 ====-\n");
		printf("-= Written by Splamy =-\n");

		if (!TryHook())
		{
			printf("Packet dispatcher not found, aborting");
			return FALSE;
		}

		CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)idle_loop, nullptr, NULL, nullptr);
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

extern "C"
{
	const char* print_in_format = "[ IN] %.*s\n";
	const char* print_out_format = "[OUT] %.*s\n";
}

#ifdef ENV32
bool TryHook()
{
	const auto match_in_1 = FindPattern(mod, MASK_IN_1, PATT_IN_1);
	if (match_in_1 != NULL)
		printf("> Found PKGIN1: %zX\n", match_in_1);

	const auto match_out_1 = FindPattern(mod, MASK_OUT_1, PATT_OUT_1);
	if (match_out_1 != NULL)
		printf("> Found PKGOUT1: %zX\n", match_out_1);

	if (match_in_1 != NULL && match_out_1 != NULL)
	{
		const SIZE_T OFFS_IN_1 = 13;
		packet_in_hook_return = match_in_1 + OFFS_IN_1 + 5;
		MakeJMP((PBYTE)(match_in_1 + OFFS_IN_1), packet_in_hook1, 5);

		const SIZE_T OFFS_OUT_1 = 33;
		packet_out_hook_return = match_out_1 + OFFS_OUT_1 + 8;
		MakeJMP((PBYTE)(match_out_1 + OFFS_OUT_1), packet_out_hook1, 8);
		return true;
	}

	return false;
}

void __declspec(naked) packet_in_hook1()
{
	__asm
	{
		// +11

		PUSHAD
		MOV eax, [esi + 4]
		ADD eax, 11
		PUSH eax // str
		MOV ecx, [esi + 8]
		SUB ecx, 11
		PUSH ecx // len
		PUSH print_in_format
		CALL printf
		ADD esp, 12
		POPAD

		// overwritten
		PUSH edi
		PUSH esi
		CALL DWORD PTR[eax + 16]
		JMP packet_in_hook_return
	}
}

void __declspec(naked) packet_out_hook1()
{
	__asm
	{
		// +13

		PUSHAD
		MOV eax, [edi]
		ADD eax, 13
		PUSH eax // str
		MOV ecx, [edi + 4]
		SUB ecx, 13
		PUSH ecx // len
		PUSH print_out_format
		CALL printf
		ADD esp, 12
		POPAD

		// overwritten
		CMP DWORD PTR[ebp + 16], 1
		SETZ BYTE PTR[ebp + 4]
		JMP packet_out_hook_return
	}
}
#else
bool TryHook()
{
	const auto match_in_1 = FindPattern(mod, MASK_IN_1, PATT_IN_1);
	if (match_in_1 != NULL)
		printf("> Found PKGIN1: %zX\n", match_in_1);

	const auto match_out_1 = FindPattern(mod, MASK_OUT_1, PATT_OUT_1);
	if (match_out_1 != NULL)
		printf("> Found PKGOUT1: %zX\n", match_out_1);

	if (match_in_1 != NULL && match_out_1 != NULL)
	{
		packet_in_hook_return = match_in_1 + 22;
		MakeJMP((PBYTE)(match_in_1), packet_in_hook1, 22);

		packet_out_hook_return = match_out_1 + 18;
		MakeJMP((PBYTE)(match_out_1), packet_out_hook1, 18);
		return true;
	}

	return false;
}
#endif

void idle_loop()
{
	while (true)
	{
		Sleep(100);
	}
}