#include "pch.h"
#include "opcodes.h"
#include <iostream>

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
                    {"eventSubscriptions", 33}
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

void SendOpCode(std::string argument, WebSocketClient& client) {
    json data;
    data["op"] = 6;
    data["d"]["requestType"] = "SetCurrentProgramScene";
    data["d"]["requestId"] = "idk whats this";
    data["d"]["requestData"]["sceneName"] = argument;

    std::string datadump = data.dump();
    client.sendMessage(datadump);
}