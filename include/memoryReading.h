#pragma once
#include <LR2Mem/LR2Bindings.hpp>
#include <websocket.h>

int LR2Listen(WebSocketClient* client);
void recordDelayTask(int currentProc, WebSocketClient& client);