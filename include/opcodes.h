#pragma once
#include "pch.h"
#include "websocket.h"
#include "settings.h"
#include "memoryReading.h"
#include <json/single_include/nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

std::string currentDateTime();
std::string sha256_binary(const std::string& input);
std::string base64_encode(const unsigned char* data, unsigned int length);
bool ReadOpCode(std::string message, WebSocketClient& client);
void SendOpCode(std::string argument, WebSocketClient& client);
