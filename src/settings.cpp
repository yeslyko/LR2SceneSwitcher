#include "pch.h"
#include "settings.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include "currentDateTime.h"

Settings settings;

#ifdef _DEBUG
FILE* stream;
BOOL consoleAllocated = FALSE;

void SetupConsole() {
    if (!consoleAllocated) {
        AllocConsole();
        freopen_s(&stream, "CONOUT$", "w", stdout);
        SetConsoleTitleA("LR2SceneSwitcher Debug Console");
        consoleAllocated = TRUE;
    }
}

void CleanupConsole() {
    if (consoleAllocated) {
        if (stream) {
            fclose(stream);
        }
        FreeConsole();
        consoleAllocated = FALSE;
    }
}
#endif

void LoadSettings(HMODULE hModule) {
#ifdef _DEBUG
	SetupConsole();
	std::cout << currentDateTime() << "Debug console initialized\n";
#endif

	TCHAR module_path[MAX_PATH] = {};
	GetModuleFileName(0, module_path, MAX_PATH);

	std::filesystem::path settings_path = std::filesystem::path(module_path).remove_filename().append("LR2SceneSwitcher.ini");
	std::cout << currentDateTime() << "Loading settings from: " << settings_path.string() << std::endl;

	std::wfstream settings_list;

	if (!std::filesystem::exists(settings_path)) {
		std::cout << currentDateTime() << "Settings file not found, creating one.\n";
		std::wstring settingstemplate =
			L"# Leave the password field empty if you are not using it for authenticating to\n"
			L"# WebSocket server.For the sake of security, I strongly recommend using password.\n"
			L"ip = \n"
			L"port = 4455\n"
			L"password = \n\n"
			L"# Type your OBS scene names here.\n"
			L"selectScene = \n"
			L"playScene = \n"
			L"resultScene = \n"
			L"# Set your record type in here.\n"
			L"recordType = 0\n";
		settings_list.open(settings_path, std::ios::out | std::ios::trunc);
		settings_list << settingstemplate;
		settings_list.close();
	}
	
	settings_list.open(settings_path, std::ios::in);
	if (settings_list.is_open()) {
		for (std::wstring line; std::getline(settings_list, line);) {
			if (line.empty() || line.starts_with(L"#"))
				continue;
			if (line.starts_with(L"ip = ")) {
				std::wstring wip = line.substr(std::wstring_view(L"ip = ").length());
				std::string newip(wip.begin(), wip.end());

				if (newip.length() == 0) {
					newip = "localhost";
					continue;
				}
				settings.ip = newip;
			}
			if (line.starts_with(L"port = ")) {
				std::wstring wport = line.substr(std::wstring_view(L"port = ").length());
				std::string newport(wport.begin(), wport.end());

				if (newport.length() == 0) {
					newport = "4455";
					continue;
				}
				settings.port = newport;
			}
			if (line.starts_with(L"password = ")) {
				std::wstring wpassword = line.substr(std::wstring_view(L"password = ").length());
				std::string password(wpassword.begin(), wpassword.end());
				//Kill me with a huge rock

				if (password.length() == 0) {
					settings.authenticate = false;
					continue;
				}
				settings.password = password;
				settings.authenticate = true;
			}
			if (line.starts_with(L"selectScene = ")) {
				std::wstring wselectScene = line.substr(std::wstring_view(L"selectScene = ").length());
				std::string selectScene(wselectScene.begin(), wselectScene.end());
				settings.selectScene = selectScene;
			}
			if (line.starts_with(L"playScene = ")) {
				std::wstring wplayScene = line.substr(std::wstring_view(L"playScene = ").length());
				std::string playScene(wplayScene.begin(), wplayScene.end());
				settings.playScene = playScene;
			}
			if (line.starts_with(L"resultScene = ")) {
				std::wstring wresultScene = line.substr(std::wstring_view(L"resultScene = ").length());
				std::string resultScene(wresultScene.begin(), wresultScene.end());
				settings.resultScene = resultScene;
			}
			if (line.starts_with(L"recordType = ")) {
				std::wstring wrecordType = line.substr(std::wstring_view(L"recordType = ").length());
				int recordType = std::stoi(wrecordType);
				settings.recordType = recordType;
			}
			// Heyy! That's still less bloated than Tachi Score Importer!!!
		}

		std::cout << currentDateTime() << "Settings loaded:"
			<< "\nIP: " << settings.ip
			<< "\nPort: " << settings.port
			<< "\nAuthentication: " << (settings.authenticate ? "Yes" : "No")
			<< "\nPassword length: " << settings.password.length()
			<< "\nSelect Scene: " << settings.selectScene
			<< "\nPlay Scene: " << settings.playScene
			<< "\nResult Scene: " << settings.resultScene
			<< "\nRecord Type: " << settings.recordType
			<< std::endl;
	}
}
