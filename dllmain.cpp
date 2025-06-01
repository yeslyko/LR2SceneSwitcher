#define WIN32_LEAN_AND_MEAN
#include "pch.h"
#include "websocket.h"
#include "settings.h"
#include "opcodes.h"
#include "memoryReading.h"
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <Windows.h>
#include <fstream>
#include <cpp-base64/base64.cpp>
#include <sha256/sha256.c>
#include <ctime>

using json = nlohmann::json;
using namespace nlohmann::literals;

#pragma comment(lib, "ws2_32.lib")

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		LoadSettings(hModule);
#ifdef _DEBUG
		if (CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)StartWebSocketClient, nullptr, 0, nullptr) == nullptr) {
			std::cout << currentDateTime() << "Failed to create WebSocket client thread\n";
			std::cout.flush();
		}
#else
		CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)StartWebSocketClient, nullptr, 0, nullptr);
#endif
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
#ifdef _DEBUG
		CleanupConsole();
#endif
		break;
	}
	return TRUE;
}