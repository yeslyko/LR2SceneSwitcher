#include "pch.h"
#include "memoryReading.h"
#include "websocket.h"
#include "currentDateTime.h"
#include "settings.h"
#include "opcodes.h"
#include <future>

BOOL hasRequestRecordingStop = false;

int LR2Listen(WebSocketClient* client) {
    std::cout << currentDateTime() << "LR2Listen started, performing initial checks...\n";

    int lastProcSelecter = -1;
    int currentProc = -1;
    int recordType = settings.recordType;
    bool wasInitialized = false;
    bool isResult = false;
   
    while (client->isConnected()) {
        if (!LR2::isInit) {
            if (wasInitialized) {
                std::cout << currentDateTime() << "LR2 initialization lost, attempting to reinitialize...\n";
                wasInitialized = false;
            }

            LR2::Init();

            if (LR2::isInit) {
                std::cout << currentDateTime() << "LR2 successfully initialized!\n";
                std::cout << currentDateTime() << "pGame address: " << std::hex << LR2::pGame
                    << ", procSelecter offset: " << std::hex << &(LR2::pGame->procSelecter) << std::dec << std::endl;
                wasInitialized = true;
            }
            else {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }
        }

        if (LR2::isInit) {
            wasInitialized = true;
            currentProc = LR2::pGame->procSelecter;
            isResult = currentProc == 5;

            if (isResult && recordType > 0) {
                if (GetAsyncKeyState(settings.recordShortcutKey) & 0x8000) {
                    if (!hasRequestRecordingStop) {
                        switch (recordType) {
                        case 1:
                            SendOpCode("StopRecord", *client);
                            break;
                        case 2:
                            SendOpCode("SaveReplayBuffer", *client);
                            break;

                        default:
                            break;
                        }
                        hasRequestRecordingStop = true;
                    }
                }
            }

            if (currentProc != lastProcSelecter) {
                std::cout << currentDateTime() << "procSelecter changed from "
                    << lastProcSelecter << " to " << currentProc << std::endl;

                switch (currentProc) {
                case 2:
                    if (settings.recordType > 0) {
                        std::cout << currentDateTime() << "Requesting stop recording.\n";
                        std::async(std::launch::async, recordDelayTask, currentProc, std::ref(*client));
                    }
                    std::cout << currentDateTime() << "Requesting change to song select scene.\n";
                    SendOpCode("SetCurrentProgramScene", settings.selectScene, *client);
                    break;
                case 3:
                    if (settings.recordType > 0) {
                        std::cout << currentDateTime() << "Requesting start recording.\n";
                        std::async(std::launch::async, recordDelayTask, currentProc, std::ref(*client));
                    }
                    break;
                case 4:
                    std::cout << currentDateTime() << "Requesting change to play scene.\n";
                    SendOpCode("SetCurrentProgramScene", settings.playScene, *client);
                    break;
                case 5:
                    std::cout << currentDateTime() << "Requesting change to result scene.\n";
                    SendOpCode("SetCurrentProgramScene", settings.resultScene, *client);
                    break;
                case 13:
                    // TODO:: COURSE RESULT
                    // transition goes to 5 -> 2 -> 13
                    break;
                default:
                    std::cout << currentDateTime() << "Unhandled procSelecter value: " << currentProc << std::endl;
                    break;
                }

                lastProcSelecter = currentProc;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}

void recordDelayTask(int currentProc, WebSocketClient& client) {
    switch (currentProc) {
    case 2:
        if (settings.recordEndDelay > 0) std::this_thread::sleep_for(std::chrono::milliseconds(settings.recordEndDelay));
        switch (settings.recordType) {
        case 1:
            if (!hasRequestRecordingStop) SendOpCode("StopRecord", client);
            else hasRequestRecordingStop = false;
            break;
        case 2:
            if (hasRequestRecordingStop) hasRequestRecordingStop = false;
            SendOpCode("StopReplayBuffer", client);
            break;

        default:
            break;
        }
        break;
    case 3:
        if (settings.recordStartDelay > 0) std::this_thread::sleep_for(std::chrono::milliseconds(settings.recordStartDelay));
        switch (settings.recordType) {
        case 1:
            SendOpCode("StartRecord", client);
            break;
        case 2:
            SendOpCode("StartReplayBuffer", client);
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }
}
