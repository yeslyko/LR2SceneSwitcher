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
void SendOpCode(std::string reqName, std::string argument, WebSocketClient& client);
inline void SendOpCode(std::string reqName, WebSocketClient& client) { SendOpCode(reqName, "", client); };

void recordRenameTask(std::string outputPath);

// from LR2HackBox ScoreCannon.cpp
constexpr const char* lamps[] = { "NO PLAY", "FAILED", "EASY CLEAR", "GROOVE CLEAR", "HARD CLEAR", "FULL COMBO", "PERFECT", "MAX", "ASSIST CLEAR", "NONE" };
constexpr const char* grades[] = { "F", "E", "D", "C", "B", "A", "AA", "AAA", "MAX" };
int getGrade();
std::string convStr(const std::string& str);
