#pragma once
#include <websocket.h>

extern bool reqRestartRecord;
extern bool isCourseResult;

int LR2Listen(WebSocketClient* client);
void onSceneInit(SafetyHookContext& regs);
void onSceneLoop(SafetyHookContext& regs);
void recordDelayTask(int sceneIdx);
