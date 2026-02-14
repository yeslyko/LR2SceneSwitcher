#pragma once
#include <websocket.h>

struct playInfo {
	std::string songName;
	int lamp;
	int totalNotes;
	int exscore;
	int pgreat;
	int great;
};

extern bool reqRestartRecord;
extern bool isCourseResult;
extern playInfo _playInfo;

int LR2Listen(WebSocketClient* client);
void onSceneInit(SafetyHookContext& regs);
void onSceneLoop(SafetyHookContext& regs);
void recordDelayTask(int sceneIdx);
