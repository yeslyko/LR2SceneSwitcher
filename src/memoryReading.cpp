#include "pch.h"
#include "memoryReading.h"
#include "websocket.h"
#include "currentDateTime.h"
#include "settings.h"
#include "opcodes.h"
#include <future>

WebSocketClient* webSocketClient = {};
bool wasInitialized = false;
bool hasRequestRecordingStop = false;
bool isCourseResult = false;
bool reqRestartRecord = false;
int currentSceneIdx = -1;
int previousSceneIdx = -1;

int LR2Listen(WebSocketClient* client) {
    std::cout << currentDateTime() << "LR2Listen started...\n";
    webSocketClient = client;

    while (client->isConnected()) {
        if (!wasInitialized) wasInitialized = true;

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    std::cout << currentDateTime() << "WebSocket connection has been lost..." << std::endl;
    wasInitialized = false;
    return 0;
}

void onSceneInit(SafetyHookContext& regs) {
    if (!wasInitialized) return;
    currentSceneIdx = regs.eax;

    switch (currentSceneIdx) {
    case 2:
        if (settings.recordType > 0) {
            std::cout << currentDateTime() << "Requesting stop recording.\n";
            std::async(std::launch::async, recordDelayTask, currentSceneIdx);
        }
        std::cout << currentDateTime() << "Requesting change to song select scene.\n";
        SendOpCode("SetCurrentProgramScene", settings.selectScene, *webSocketClient);
        break;
    case 3:
        if (settings.recordType > 0) {
            std::cout << currentDateTime() << "Requesting start recording.\n";
            std::async(std::launch::async, recordDelayTask, currentSceneIdx);
        }
        break;
    case 4:
        std::cout << currentDateTime() << "Requesting change to play scene.\n";
        if (settings.recordType > 0) {
            if (previousSceneIdx == currentSceneIdx || previousSceneIdx == 5) { // quick restart or restart //
                std::cout << currentDateTime() << "Restart record request detected\n";
                reqRestartRecord = true;

                switch (settings.recordType) {
                case 1:
                    SendOpCode("StopRecord", *webSocketClient);
                    break;
                case 2:
                    SendOpCode("StopReplayBuffer", *webSocketClient);
                    break;

                default:
                    break;
                }
            }
        }
        SendOpCode("SetCurrentProgramScene", settings.playScene, *webSocketClient);
        break;
    case 5:
        std::cout << currentDateTime() << "Requesting change to result scene.\n";
        SendOpCode("SetCurrentProgramScene", settings.resultScene, *webSocketClient);
        break;
    case 13:
        std::cout << currentDateTime() << "Requesting change to course result scene.\n";
        SendOpCode("SetCurrentProgramScene", settings.courseResultScene, *webSocketClient);
        isCourseResult = true;
        break;

    default:
        std::cout << currentDateTime() << "Unhandled scene index value: " << currentSceneIdx << std::endl;
        break;
    }

    previousSceneIdx = currentSceneIdx;
}

void onSceneLoop(SafetyHookContext& regs) {
    switch (regs.eax) {
    case 5:
    case 13:
        if (settings.recordType > 0) {
            if (GetAsyncKeyState(settings.recordShortcutKey) & 0x8000) {
                if (!hasRequestRecordingStop) {
                    switch (settings.recordType) {
                    case 1:
                        SendOpCode("StopRecord", *webSocketClient);
                        break;
                    case 2:
                        SendOpCode("SaveReplayBuffer", *webSocketClient);
                        break;

                    default:
                        break;
                    }
                    hasRequestRecordingStop = true;
                }
            }
        }
        break;

    default:
        break;
    }
}

void recordDelayTask(int sceneIdx) {
    switch (sceneIdx) {
    case 2:
    case 13:
        if (settings.recordEndDelay > 0) std::this_thread::sleep_for(std::chrono::milliseconds(settings.recordEndDelay));
        switch (settings.recordType) {
        case 1:
            if (!hasRequestRecordingStop) SendOpCode("StopRecord", *webSocketClient);
            else hasRequestRecordingStop = false;
            break;
        case 2:
            if (hasRequestRecordingStop) hasRequestRecordingStop = false;
            SendOpCode("StopReplayBuffer", *webSocketClient);
            break;

        default:
            break;
        }
        break;
    case 3:
        if (settings.recordStartDelay > 0) std::this_thread::sleep_for(std::chrono::milliseconds(settings.recordStartDelay));
        switch (settings.recordType) {
        case 1:
            SendOpCode("StartRecord", *webSocketClient);
            break;
        case 2:
            SendOpCode("StartReplayBuffer", *webSocketClient);
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }
}
