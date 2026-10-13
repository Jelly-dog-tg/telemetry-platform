#pragma once

#include <string>

enum class MessageType {
    Register,
    Heartbeat,
    Telemetry
};

struct ProtocolMessage {
    MessageType type = MessageType::Telemetry;

    int deviceId = 0;
    double temperature = 0.0;
    bool online = false;
    int sequence = 0;
};

bool parseProtocolMessage(
    const std::string& line,
    ProtocolMessage& message,
    std::string& error
);
