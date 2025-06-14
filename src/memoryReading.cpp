#include "pch.h"
#include "memoryReading.h"
#include "websocket.h"
#include "currentDateTime.h"
#include "settings.h"
#include "opcodes.h"

DWORD WINAPI LR2Listen(LPVOID lpParam) {
    WebSocketClient* client = static_cast<WebSocketClient*>(lpParam);

    std::cout << currentDateTime() << "LR2Listen started, performing initial checks...\n";

    int lastProcSelecter = -1;
    bool wasInitialized = false;

    while (true) {
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
                Sleep(2000);
                continue;
            }
        }

        if (LR2::isInit) {
            wasInitialized = true;
            int currentProc = LR2::pGame->procSelecter;

            if (currentProc != lastProcSelecter) {
                std::cout << currentDateTime() << "procSelecter changed from "
                    << lastProcSelecter << " to " << currentProc << std::endl;

                switch (currentProc) {
                case 2:
                    std::cout << currentDateTime() << "Requesting change to song select scene.\n";
                    SendOpCode(settings.selectScene, *client);
                    break;
                case 4:
                    std::cout << currentDateTime() << "Requesting change to play scene.\n";
                    SendOpCode(settings.playScene, *client);
                    break;
                case 5:
                    std::cout << currentDateTime() << "Requesting change to result scene.\n";
                    SendOpCode(settings.resultScene, *client);
                    break;
                default:
                    std::cout << currentDateTime() << "Unhandled procSelecter value: " << currentProc << std::endl;
                    break;
                }

                lastProcSelecter = currentProc;
            }
        }

        Sleep(500);
    }

    return 0;
}