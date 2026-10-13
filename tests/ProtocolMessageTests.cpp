#include "ProtocolMessage.h"

#include <iostream>
#include <string>

int passed = 0;
int failed = 0;

void check(bool condition, const std::string& name) {
    if (condition) {
        ++passed;
        std::cout << "[PASS] " << name << '\n';
    } else {
        ++failed;
        std::cerr << "[FAIL] " << name << '\n';
    }
}

void expectRejected(
    const std::string& input,
    const std::string& name
) {
    ProtocolMessage message;
    std::string error;

    bool result = parseProtocolMessage(
        input,
        message,
        error
    );

    check(!result && !error.empty(), name);
}

int main() {
    // TELEMETRY
    {
        ProtocolMessage message;
        std::string error;

        bool result = parseProtocolMessage(
            "TELEMETRY,1,36.5,1,5\n",
            message,
            error
        );

        check(result, "Valid telemetry is accepted");

        check(
            message.type == MessageType::Telemetry &&
            message.deviceId == 1 &&
            message.temperature == 36.5 &&
            message.online &&
            message.sequence == 5,
            "Telemetry fields are correct"
        );
    }

    {
        ProtocolMessage message;
        std::string error;

        check(
            parseProtocolMessage(
                "TELEMETRY,1,92.5,1,6",
                message,
                error
            ) &&
            message.temperature == 92.5,
            "High temperature is syntactically valid"
        );
    }

    expectRejected(
        "TELEMETRY,0,36.5,1,1",
        "Zero device ID is rejected"
    );

    expectRejected(
        "TELEMETRY,1,36.5abc,1,1",
        "Invalid temperature text is rejected"
    );

    expectRejected(
        "TELEMETRY,1,nan,1,1",
        "NaN temperature is rejected"
    );

    expectRejected(
        "TELEMETRY,1,inf,1,1",
        "Infinite temperature is rejected"
    );

    expectRejected(
        "TELEMETRY,1,36.5,2,1",
        "Invalid online status is rejected"
    );

    expectRejected(
        "TELEMETRY,1,36.5,1,0",
        "Zero telemetry sequence is rejected"
    );

    expectRejected(
        "TELEMETRY,1,36.5,1",
        "Missing telemetry field is rejected"
    );

    expectRejected(
        "TELEMETRY,1,36.5,1,1,extra",
        "Extra telemetry field is rejected"
    );

    // REGISTER
    {
        ProtocolMessage message;
        std::string error;

        check(
            parseProtocolMessage(
                "REGISTER,100\n",
                message,
                error
            ) &&
            message.type == MessageType::Register &&
            message.deviceId == 100,
            "Valid registration is accepted"
        );
    }

    expectRejected(
        "REGISTER,0",
        "Registration with zero ID is rejected"
    );

    expectRejected(
        "REGISTER,100,extra",
        "Registration with extra fields is rejected"
    );

    // HEARTBEAT
    {
        ProtocolMessage message;
        std::string error;

        check(
            parseProtocolMessage(
                "HEARTBEAT,100,7",
                message,
                error
            ) &&
            message.type == MessageType::Heartbeat &&
            message.deviceId == 100 &&
            message.sequence == 7,
            "Valid heartbeat is accepted"
        );
    }

    expectRejected(
        "HEARTBEAT,100,0",
        "Heartbeat with zero sequence is rejected"
    );

    expectRejected(
        "HEARTBEAT,100",
        "Heartbeat with missing field is rejected"
    );

    expectRejected(
        "UNKNOWN,1,2",
        "Unknown message type is rejected"
    );

    expectRejected(
        "",
        "Empty message is rejected"
    );

    std::cout << "\nPassed: " << passed
              << ", Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
