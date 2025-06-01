#pragma once

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <string>
#include <vector>

void StartWebSocketClient();

class WebSocketClient {
private:
    SOCKET sock;
    bool connected;
    static const size_t BUFFER_SIZE = 1024;
    static const uint8_t OPCODE_CLOSE = 0x8;
    static const uint8_t OPCODE_TEXT = 0x1;

    struct WebSocketFrame {
        bool isFinalFragment;
        uint8_t opcode;
        std::string payload;
        uint16_t closeCode;

        WebSocketFrame() : isFinalFragment(false), opcode(0), closeCode(0) {}
    };

    enum class WebSocketCloseCode {
        DontClose = 0,
        Normal = 1000,
        GoingAway = 1001,
        ProtocolError = 1002,
        UnsupportedData = 1003,
        Unknown = 1005,
        AbnormalClose = 1006,
        InvalidPayload = 1007,
        PolicyViolation = 1008,
        MessageTooBig = 1009,
        InternalError = 1011,
        // OBS WebSocket specific codes
        DontConnect = 4000,
        MissingDataField = 4002,
        InvalidDataFieldType = 4003,
        InvalidDataFieldValue = 4004,
        UnknownOpCode = 4005,
        UnsupportedRpcVersion = 4006,
        NotIdentified = 4007,
        AlreadyIdentified = 4008,
        AuthenticationFailed = 4009,
        UnsupportedFeature = 4010,
        SessionInvalidated = 4011,
        UnsupportedEncoding = 4012
    };

    std::string getCloseCodeMessage(uint16_t code);
    bool performHandshake(const std::string& host, const std::string& path);
    WebSocketFrame decodeWebSocketFrame(const char* buffer, size_t length);
    std::vector<uint8_t> encodeWebSocketFrame(const std::string& message);

public:
    WebSocketClient();
    ~WebSocketClient();

    bool connect(const std::string& host, const std::string& port);
    std::string receiveMessage();
    bool sendMessage(const std::string& message);
    bool isConnected() const;
};