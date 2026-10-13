#include "ProtocolMessage.h"

#include <cctype>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

bool containsWhitespace(const std::string& value) {
    for (unsigned char ch : value) {
        if (std::isspace(ch)) {
            return true;
        }
    }

    return false;
}

int parseIntegerField(const std::string& value) {
    if (value.empty() || containsWhitespace(value)) {
        throw std::invalid_argument("invalid integer field");
    }

    std::size_t pos = 0;
    int result = std::stoi(value, &pos);

    if (pos != value.size()) {
        throw std::invalid_argument("invalid integer field");
    }

    return result;
}

double parseDoubleField(const std::string& value) {
    if (value.empty() || containsWhitespace(value)) {
        throw std::invalid_argument("invalid temperature field");
    }

    std::size_t pos = 0;
    double result = std::stod(value, &pos);

    if (pos != value.size()) {
        throw std::invalid_argument("invalid temperature field");
    }

    return result;
}

}  // namespace

bool parseProtocolMessage(
    const std::string& rawLine,
    ProtocolMessage& message,
    std::string& error
) {
    error.clear();

    std::string line = rawLine;

    // Remove line endings.
    if (!line.empty() && line.back() == '\n') {
        line.pop_back();
    }

    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    if (line.empty()) {
        error = "empty message";
        return false;
    }

    // Split fields.
    std::vector<std::string> fields;
    std::size_t start = 0;

    while (true) {
        std::size_t comma = line.find(',', start);

        if (comma == std::string::npos) {
            fields.push_back(line.substr(start));
            break;
        }

        fields.push_back(line.substr(start, comma - start));
        start = comma + 1;
    }

    ProtocolMessage parsed;
    std::size_t expectedFields = 0;

    // Determine message type and expected field count.
    if (fields[0] == "REGISTER") {
        parsed.type = MessageType::Register;
        expectedFields = 2;
    } else if (fields[0] == "HEARTBEAT") {
        parsed.type = MessageType::Heartbeat;
        expectedFields = 3;
    } else if (fields[0] == "TELEMETRY") {
        parsed.type = MessageType::Telemetry;
        expectedFields = 5;
    } else {
        error = "unknown message type";
        return false;
    }

    if (fields.size() != expectedFields) {
        error = "incorrect number of fields";
        return false;
    }

    for (const std::string& field : fields) {
        if (field.empty()) {
            error = "empty field";
            return false;
        }
    }

    try {
        parsed.deviceId = parseIntegerField(fields[1]);

        if (parsed.deviceId <= 0) {
            error = "device ID must be positive";
            return false;
        }

        if (parsed.type == MessageType::Register) {
            parsed.online = true;
            parsed.sequence = 0;
        } else if (parsed.type == MessageType::Heartbeat) {
            parsed.sequence = parseIntegerField(fields[2]);

            if (parsed.sequence <= 0) {
                error = "sequence must be positive";
                return false;
            }

            parsed.online = true;
        } else {
            parsed.temperature = parseDoubleField(fields[2]);

            int onlineValue = parseIntegerField(fields[3]);
            parsed.sequence = parseIntegerField(fields[4]);

            if (!std::isfinite(parsed.temperature)) {
                error = "temperature must be finite";
                return false;
            }

            if (onlineValue != 0 && onlineValue != 1) {
                error = "online status must be 0 or 1";
                return false;
            }

            if (parsed.sequence <= 0) {
                error = "sequence must be positive";
                return false;
            }

            parsed.online = (onlineValue == 1);
        }
    } catch (const std::exception&) {
        error = "invalid numeric field";
        return false;
    }

    message = parsed;
    return true;
}
