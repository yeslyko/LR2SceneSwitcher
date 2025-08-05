#include "pch.h"
#include "websocket.h"
#include "opcodes.h"
#include <ws2tcpip.h>
#include <iostream>
#include "currentDateTime.h"
#include "settings.h"

WebSocketClient::WebSocketClient() : sock(INVALID_SOCKET), connected(false) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << currentDateTime() << "WSAStartup failed with error: " << WSAGetLastError() << std::endl;
        return;
    }
    srand(static_cast<unsigned int>(time(nullptr)));
}

WebSocketClient::~WebSocketClient() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }
    WSACleanup();
}

std::string WebSocketClient::getCloseCodeMessage(uint16_t code) {
    switch (static_cast<WebSocketCloseCode>(code)) {
        case WebSocketCloseCode::UnsupportedRpcVersion:
            return "Unsupported RPC Version";
        case WebSocketCloseCode::AuthenticationFailed:
            return "Authentication Failed";
        case WebSocketCloseCode::NotIdentified:
            return "Not Identified";
        case WebSocketCloseCode::AlreadyIdentified:
            return "Already Identified";
        case WebSocketCloseCode::SessionInvalidated:
            return "Session Invalidated";
        default:
            return "Unknown Close Code: " + std::to_string(code);
    }
}

bool WebSocketClient::performHandshake(const std::string& host, const std::string& path) {
    std::string key = "dGhlIHNhbXBsZSBub25jZQ==";
    
    std::string handshake = 
        "GET " + path + " HTTP/1.1\r\n"
        "Host: " + host + "\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: " + key + "\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Protocol: obswebsocket.json\r\n"
        "\r\n";

    std::cout << currentDateTime() << "Sending handshake request...\n";

    if (send(sock, handshake.c_str(), handshake.length(), 0) < 0) {
        std::cout << currentDateTime() << "Failed to send handshake. Error: " << WSAGetLastError() << std::endl;
        return false;
    }

    char buffer[1024];
    int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes < 0) {
        std::cout << currentDateTime() << "Failed to receive handshake response. Error: " << WSAGetLastError() << std::endl;
        return false;
    }
    buffer[bytes] = 0;

    std::cout << currentDateTime() << "Received handshake response: " << buffer << std::endl;

    return strstr(buffer, "101 Switching Protocols") != nullptr;
}

bool WebSocketClient::connect(const std::string& host, const std::string& port) {
    struct addrinfo hints, *result;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &result) != 0) {
        std::cerr << currentDateTime() << "Failed to resolve host: " << WSAGetLastError() << std::endl;
        return false;
    }

    sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        std::cerr << currentDateTime() << "Failed to create socket: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        return false;
    }

    if (::connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        std::cerr << currentDateTime() << "Failed to connect: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        freeaddrinfo(result);
        return false;
    }

    freeaddrinfo(result);

    connected = performHandshake(host, "/");
    return connected;
}

std::string WebSocketClient::receiveMessage() {
    char buffer[BUFFER_SIZE];
    int bytes = recv(sock, buffer, sizeof(buffer), 0);
    
    if (bytes <= 0) {
        if (bytes == 0) {
            connected = false;
            std::cout << currentDateTime() << "Connection closed by peer\n";
        } else {
            std::cerr << currentDateTime() << "Error receiving data: " << WSAGetLastError() << std::endl;
        }
        return "";
    }

    WebSocketFrame frame = decodeWebSocketFrame(buffer, bytes);
    
    if (frame.opcode == OPCODE_CLOSE) {
        connected = false;
        closesocket(sock);
        std::string closeMsg = getCloseCodeMessage(frame.closeCode);
        std::cout << currentDateTime() << "Connection closed: " << closeMsg << std::endl;
        return "";
    }
    
    return frame.payload;
}

bool WebSocketClient::sendMessage(const std::string& message) {
    if (!connected) {
        return false;
    }

    std::vector<uint8_t> frame = encodeWebSocketFrame(message);
    int bytes = send(sock, reinterpret_cast<const char*>(frame.data()), frame.size(), 0);
    
    if (bytes == SOCKET_ERROR) {
        std::cerr << currentDateTime() << "Failed to send message: " << WSAGetLastError() << std::endl;
        return false;
    }
    
    return true;
}

bool WebSocketClient::isConnected() const {
    return connected;
}

WebSocketClient::WebSocketFrame WebSocketClient::decodeWebSocketFrame(const char* buffer, size_t length) {
    WebSocketFrame frame;
    if (length < 2) return frame;

    const uint8_t* data = reinterpret_cast<const uint8_t*>(buffer);
    frame.isFinalFragment = (data[0] & 0x80) != 0;
    frame.opcode = data[0] & 0x0F;
    bool masked = (data[1] & 0x80) != 0;
    uint64_t payloadLength = data[1] & 0x7F;
    size_t headerOffset = 2;

    if (payloadLength == 126) {
        payloadLength = (data[2] << 8) | data[3];
        headerOffset += 2;
    } else if (payloadLength == 127) {
        payloadLength = 0;
        for (int i = 0; i < 8; i++) {
            payloadLength = (payloadLength << 8) | data[2 + i];
        }
        headerOffset += 8;
    }

    uint32_t mask = 0;
    if (masked) {
        mask = *reinterpret_cast<const uint32_t*>(data + headerOffset);
        headerOffset += 4;
    }

    std::vector<char> payload(data + headerOffset, data + headerOffset + payloadLength);
    if (masked) {
        for (size_t i = 0; i < payload.size(); i++) {
            payload[i] ^= reinterpret_cast<uint8_t*>(&mask)[i % 4];
        }
    }

    if (frame.opcode == OPCODE_CLOSE && payload.size() >= 2) {
        frame.closeCode = (static_cast<uint16_t>(static_cast<uint8_t>(payload[0])) << 8) |
                          static_cast<uint8_t>(payload[1]);
    }

    frame.payload = std::string(payload.begin(), payload.end());
    return frame;
}

std::vector<uint8_t> WebSocketClient::encodeWebSocketFrame(const std::string& message) {
    std::vector<uint8_t> frame;
    uint8_t firstByte = 0x81; // FIN bit set, text frame
    frame.push_back(firstByte);

    // Generate random 4-byte mask
    uint8_t mask[4];
    for (int i = 0; i < 4; i++) {
        mask[i] = static_cast<uint8_t>(rand() & 0xFF);
    }

    size_t length = message.length();
    if (length <= 125) {
        frame.push_back(static_cast<uint8_t>(length | 0x80)); // Set mask bit
    } else if (length <= 65535) {
        frame.push_back(126 | 0x80); // Set mask bit
        frame.push_back(static_cast<uint8_t>(length >> 8));
        frame.push_back(static_cast<uint8_t>(length & 0xFF));
    } else {
        frame.push_back(127 | 0x80); // Set mask bit
        for (int i = 7; i >= 0; i--) {
            frame.push_back(static_cast<uint8_t>((length >> (i * 8)) & 0xFF));
        }
    }

    // Add mask bytes
    frame.push_back(mask[0]);
    frame.push_back(mask[1]);
    frame.push_back(mask[2]);
    frame.push_back(mask[3]);

    // Add masked message data
    for (size_t i = 0; i < message.length(); i++) {
        frame.push_back(message[i] ^ mask[i % 4]);
    }

    return frame;
}

void StartWebSocketClient() {
    WebSocketClient client;

    int retryCount = 0;
    bool connected = false;

    while (!connected && retryCount < 10) {
        std::cout << currentDateTime() << "Attempting to connect to OBS WebSocket server at "
            << settings.ip << ":" << settings.port
            << (retryCount > 0 ? " (Retry " + std::to_string(retryCount) + ")" : "") << std::endl;

        if (client.connect(settings.ip, settings.port)) {
            connected = true;
            std::cout << currentDateTime() << "Connected to WebSocket server\n";

            if (settings.authenticate) {
                std::cout << currentDateTime() << "Authentication required\n";
            }

            while (client.isConnected()) {
                std::string message = client.receiveMessage();
                if (!message.empty()) {
                    std::cout << currentDateTime() << "Received: " << message << std::endl;
                    if (!ReadOpCode(message, client)) {
                        std::cout << currentDateTime() << "Failed to process message\n";
                        break;
                    }
                }
            }
            std::cout << currentDateTime() << "WebSocket connection ended\n";
            connected = false;
        }
        else {
            std::cout << currentDateTime() << "Failed to connect to WebSocket server\n";
            if (retryCount < 9) {
                std::cout << currentDateTime() << "Retrying in 10 seconds...\n";
                Sleep(10000);
            }
            retryCount++;
        }
    }

    if (!connected) {
        std::cout << currentDateTime() << "Failed to connect after " << 5 << " attempts\n";
    }
}