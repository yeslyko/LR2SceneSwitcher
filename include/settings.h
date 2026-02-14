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
    std::string courseResultScene;
    int rpcVersion;
    bool authenticated;
    int recordType = 0; // 0 - Disable, 1 - Record, 2 - ReplayBuffer
    int recordShortcutKey = 0x75; // shortcut key for recording (stop in Record, save in ReplayBuffer)
    int recordStartDelay = 0; // delay start recording (in milliseconds)
    int recordEndDelay = 0; // delay stop recording (in milliseconds)
};

extern Settings settings;

// Function declarations
void LoadSettings(HMODULE hModule);

#ifdef _DEBUG
void SetupConsole();
void CleanupConsole();
#endif