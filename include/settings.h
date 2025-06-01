#pragma once
#include <string>
#include <Windows.h>

// Settings structure declaration
struct Settings {
    std::string ip;
    std::string port;
    std::string password;
    bool authenticate;
    std::string selectScene;
    std::string playScene;
    std::string resultScene;
    int rpcVersion;
    bool authenticated;
};

extern Settings settings;

// Function declarations
void LoadSettings(HMODULE hModule);

#ifdef _DEBUG
void SetupConsole();
void CleanupConsole();
#endif