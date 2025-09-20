#include "pch.h"
#include "opcodes.h"
#include "memoryReading.h"
#include "uuid.h"
#include <iostream>
#include <future>

std::string sha256_binary(const std::string& input) {
    SHA256 ctx;
    struct sha256_buff buff;
    sha256_init(&buff);  // Initialize the buffer
    sha256_update(&buff, (const uint8_t*)input.c_str(), input.length());  // Update with input data
    sha256_finalize(&buff);  // Finalize the hash

    unsigned char hash[32];  // SHA256 produces 32 bytes
    sha256_read(&buff, hash);  // Read the final hash

    // Convert the raw bytes to string
    return std::string(reinterpret_cast<char*>(hash), 32);
}

std::string base64_encode(const unsigned char* data, unsigned int length) {
    return base64_encode(std::string((char*)data, length));
}

bool ReadOpCode(std::string message, WebSocketClient& client) {
    try {
        json msgSerialized = json::parse(message);
        if (!msgSerialized.contains("op") || !msgSerialized.contains("d")) {
            std::cout << currentDateTime() << "Invalid message format received\n";
            return false;
        }

        int opcode = msgSerialized["op"];
        json request;

        switch (opcode) {
        case 0: // Hello message
            request = {
                {"op", 1},  // Identify operation
                {"d", {
                    {"eventSubscriptions", 65}
                }}
            };
            request["d"]["rpcVersion"] = msgSerialized["d"]["rpcVersion"];

            if (settings.authenticate) {
                if (!msgSerialized["d"].contains("authentication")) {
                    std::cout << currentDateTime() << "Missing authentication object in Hello message\n";
                    return false;
                }

                auto auth = msgSerialized["d"]["authentication"];
                if (!auth.contains("challenge") || !auth.contains("salt")) {
                    std::cout << currentDateTime() << "Invalid authentication object format\n";
                    return false;
                }

                std::string challenge = auth["challenge"].get<std::string>();
                std::string salt = auth["salt"].get<std::string>();

                try {
                    // Step 1 & 2: Create base64 secret from password + salt
                    std::string passwordAndSalt = settings.password + salt;
                    std::string binaryHash = sha256_binary(passwordAndSalt);
                    std::string base64Secret = base64_encode(binaryHash, 0);

                    // Step 3 & 4: Create authentication string from base64 secret + challenge
                    std::string secretAndChallenge = base64Secret + challenge;
                    std::string finalBinaryHash = sha256_binary(secretAndChallenge);
                    std::string authenticationString = base64_encode(finalBinaryHash, 0);

                    // Add authentication to the request
                    request["d"]["authentication"] = authenticationString;

                    std::cout << currentDateTime() << "Authentication string generated successfully\n";
                }
                catch (const std::exception& e) {
                    std::cout << currentDateTime() << "Error generating authentication string: " << e.what() << std::endl;
                    return false;
                }
            }

            std::cout << currentDateTime() << "Sending identify message: " << request.dump(2) << std::endl;
            return client.sendMessage(request.dump());

        case 2:
            settings.rpcVersion = msgSerialized["d"]["negotiatedRpcVersion"];
            settings.authenticated = true;
            std::cout << currentDateTime() << "Authenticated successfully. Negotiated RPC version: " << settings.rpcVersion << std::endl;

            client.startSendThread(LR2Listen);
            return true;

        case 5: { // event //
            std::string eventType = msgSerialized["d"]["eventType"];
            if (eventType.compare("ReplayBufferStateChanged") == 0) {
                if (!reqRestartRecord) return true;
                if (msgSerialized["d"]["eventData"]["outputActive"] == true) return true;
                
                std::string outputState = msgSerialized["d"]["eventData"]["outputState"];
                if (outputState.compare("OBS_WEBSOCKET_OUTPUT_STOPPED") != 0) return true;

                std::this_thread::sleep_for(std::chrono::milliseconds(50)); // for some reason even we waited for STOPPED event and send start request it won't start
                
                SendOpCode("StartReplayBuffer", client);
                reqRestartRecord = false;
            }
            else if (eventType.compare("RecordStateChanged") == 0) {
                if (msgSerialized["d"]["eventData"]["outputActive"] == true) return true;

                std::string outputState = msgSerialized["d"]["eventData"]["outputState"];
                if (outputState.compare("OBS_WEBSOCKET_OUTPUT_STOPPED") == 0) {
                    if (reqRestartRecord) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // same reason as above

                        SendOpCode("StartRecord", client);
                        reqRestartRecord = false;

                        return true;
                    }
                    
                    std::async(std::launch::async, recordRenameTask, msgSerialized["d"]["eventData"]["outputPath"]);
                }
            }
            else if (eventType.compare("ReplayBufferSaved") == 0) {
                std::async(std::launch::async, recordRenameTask, msgSerialized["d"]["eventData"]["savedReplayPath"]);
            }
            return true;
        }

        default:
            std::cout << currentDateTime() << "Unhandled opcode: " << opcode << std::endl;
            return true;
        }
    }
    catch (const json::exception& e) {
        std::cout << currentDateTime() << "JSON error: " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception& e) {
        std::cout << currentDateTime() << "Error: " << e.what() << std::endl;
        return false;
    }
}

void SendOpCode(std::string reqName, std::string argument, WebSocketClient& client) {
    json data;
    data["op"] = 6;
    data["d"]["requestType"] = reqName;
    data["d"]["requestId"] = uuid::generate_uuid_v4();
    if (reqName._Equal("SetCurrentProgramScene") && !argument.empty()) {
        data["d"]["requestData"]["sceneName"] = argument;
    }

    std::string datadump = data.dump();
    client.sendMessage(datadump);
}

BOOL recordRename = false;
void recordRenameTask(std::string outputPath) {
    if (recordRename) return;
    if (outputPath.empty()) return;
    if (!LR2::isInit) return;

    recordRename = true;
    while (recordRename) {
        std::filesystem::path p{ outputPath };
        
        std::string oPath = p.parent_path().string();
        std::string oExtension = p.extension().string();
        std::string rFullPath = "";
        std::string songName = convStr(LR2::pGame->sSelect.metaSelected.title.body);

        if ((LR2::pGame->gameplay.courseType == 0 || LR2::pGame->gameplay.courseType == 2) && !isCourseResult)
            songName = convStr(LR2::pGame->sSelect.bmsList[LR2::pGame->sSelect.cur_song].courseTitle[LR2::pGame->gameplay.courseStageNow].body);

        rFullPath = std::format("{}/{} [{} - {} - {}]{}",
            oPath,

            songName,
            lamps[LR2::pGame->gameplay.player[0].clearType],
            grades[getGrade()],
            LR2::pGame->gameplay.player[0].exscore,

            oExtension);

        try {
            if (std::filesystem::exists(outputPath)) {
                if (std::filesystem::exists(rFullPath)) return;
                std::filesystem::rename(outputPath, rFullPath);

                if (std::filesystem::exists(rFullPath)) {
                    std::cout << currentDateTime() << "Renamed " << outputPath << " to " << rFullPath << std::endl;
                    recordRename = false;

                    if (isCourseResult) isCourseResult = false;
                }
            }
        }
        catch (const std::exception& e) {
            std::cout << currentDateTime() << "Error: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int getGrade() {
    float score = (float)(LR2::pGame->gameplay.player[0].judgecount[5] * 2 +
        LR2::pGame->gameplay.player[0].judgecount[4]);
    float scoreMax = float(LR2::pGame->gameplay.player[0].totalnotes * 2);

    if (score == scoreMax) {
        return 8;
    }
    else if (score / scoreMax >= 8.f / 9.f) {
        return 7;
    }
    else if (score / scoreMax >= 7.f / 9.f) {
        return 6;
    }
    else if (score / scoreMax >= 6.f / 9.f) {
        return 5;
    }
    else if (score / scoreMax >= 5.f / 9.f) {
        return 4;
    }
    else if (score / scoreMax >= 4.f / 9.f) {
        return 3;
    }
    else if (score / scoreMax >= 3.f / 9.f) {
        return 2;
    }
    else if (score / scoreMax >= 2.f / 9.f) {
        return 1;
    }
    else {
        return 0;
    }
}

std::string convStr(const std::string& str) {
    if (str.empty()) return {};

    int wSize = MultiByteToWideChar(932, 0, str.c_str(),
        (int)str.size(), nullptr, 0);
    if (wSize == 0) return {};
    std::wstring wide(wSize, 0);
    MultiByteToWideChar(932, 0, str.c_str(),
        (int)str.size(), wide.data(), wSize);

    int oSize = WideCharToMultiByte(CP_ACP, 0, wide.c_str(),
        (int)wide.size(), nullptr, 0,
        nullptr, nullptr);
    if (oSize == 0) return {};
    std::string result(oSize, 0);
    WideCharToMultiByte(CP_ACP, 0, wide.c_str(),
        (int)wide.size(), result.data(), oSize,
        nullptr, nullptr);

    std::string invalid = R"(\/:*?"<>|)";
    result.erase(std::remove_if(result.begin(), result.end(),
        [&invalid](char c) { return invalid.find(c) != std::string::npos; }),
        result.end());

    return result;
}
