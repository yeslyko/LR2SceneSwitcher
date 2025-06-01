#pragma once
#include "pch.h"
#include "websocket.h"
#include "settings.h"
#include "memoryReading.h"
#include <nlohmann/json.hpp>
#include <string>
#include <sha256/sha256.h>

using json = nlohmann::json;

// Forward declarations
std::string currentDateTime();
std::string sha256_binary(const std::string& input);
std::string base64_encode(const unsigned char* data, unsigned int length);
bool ReadOpCode(std::string message, WebSocketClient& client);
void SendOpCode(std::string argument, WebSocketClient& client);

// Declare thread function used by WebSocket
DWORD WINAPI LR2Listen(LPVOID lpParam);