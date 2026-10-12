#include <arpa/inet.h>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include "CsvStorage.h"
#include "Device.h"
#include "DeviceManager.h"
#include <vector>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <cerrno>
#include <sys/time.h>

// Send the complete response, including handling partial sends.
bool sendAll(int socketFd, const std::string& data) {
    std::size_t totalSent = 0;

    while (totalSent < data.size()) {
        ssize_t sent = send(
            socketFd,
            data.data() + totalSent,
            data.size() - totalSent,
            MSG_NOSIGNAL
        );

        if (sent <= 0) {
            std::perror("send");
            return false;
        }

        totalSent += static_cast<std::size_t>(sent);
    }

    return true;
}

// Receive one complete newline-terminated message.
bool receiveLine(int clientFd, std::string& buffer) {
    constexpr std::size_t maxMessageSize = 1024;
    char chunk[256];

    while (buffer.size() < maxMessageSize) {
        ssize_t received = recv(
            clientFd,
            chunk,
            sizeof(chunk),
            0
        );

if (received == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        std::cerr << "Client receive timed out\n";
    } else {
        std::perror("recv");
    }

    return false;
}
        if (received == 0) {
            std::cerr
                << "Client disconnected before completing the message\n";
            return false;
        }

        buffer.append(
            chunk,
            static_cast<std::size_t>(received)
        );

        std::size_t newlinePos = buffer.find('\n');

        if (newlinePos != std::string::npos) {
            if (newlinePos + 1 > maxMessageSize) {
                std::cerr << "Telemetry message is too large\n";
                return false;
            }

            buffer.resize(newlinePos + 1);
            return true;
        }

        if (buffer.size() >= maxMessageSize) {
            std::cerr
                << "Telemetry message is too large or incomplete\n";
            return false;
        }
    }

    return false;
}

// Process one client connection.
void handleClient(
    int clientFd,
    std::vector<Device>& devices,
    std::mutex& devicesMutex
) {    std::string buffer;

    if (!receiveLine(clientFd, buffer)) {
        sendAll(
            clientFd,
            "ERROR: incomplete or oversized message\n"
        );
        return;
    }

    // Remove the line ending.
    if (!buffer.empty() && buffer.back() == '\n') {
        buffer.pop_back();
    }

    if (!buffer.empty() && buffer.back() == '\r') {
        buffer.pop_back();
    }

    // Split the message into five fields.
    std::istringstream messageStream(buffer);

    std::string messageType;
    std::string deviceIdText;
    std::string temperatureText;
    std::string onlineText;
    std::string sequenceText;

    bool formatOk =
        static_cast<bool>(
            std::getline(messageStream, messageType, ',')
        ) &&
        static_cast<bool>(
            std::getline(messageStream, deviceIdText, ',')
        ) &&
        static_cast<bool>(
            std::getline(messageStream, temperatureText, ',')
        ) &&
        static_cast<bool>(
            std::getline(messageStream, onlineText, ',')
        ) &&
        static_cast<bool>(
            std::getline(messageStream, sequenceText)
        );

    if (!formatOk || messageType != "TELEMETRY") {
        std::cerr << "Invalid telemetry message format\n";
        sendAll(clientFd, "ERROR: invalid message format\n");
        return;
    }

    try {
        // Convert strings into numeric values.
        std::size_t pos = 0;

        int deviceId = std::stoi(deviceIdText, &pos);
        if (pos != deviceIdText.size()) {
            throw std::invalid_argument("invalid device ID");
        }

        pos = 0;
        double temperature = std::stod(temperatureText, &pos);
        if (pos != temperatureText.size()) {
            throw std::invalid_argument("invalid temperature");
        }

        pos = 0;
        int onlineValue = std::stoi(onlineText, &pos);
        if (pos != onlineText.size()) {
            throw std::invalid_argument("invalid online status");
        }

        pos = 0;
        int sequence = std::stoi(sequenceText, &pos);
        if (pos != sequenceText.size()) {
            throw std::invalid_argument("invalid sequence");
        }

        // Validate the fields.
        if (deviceId <= 0) {
            throw std::invalid_argument(
                "device ID must be positive"
            );
        }

        if (!std::isfinite(temperature)) {
            throw std::invalid_argument(
                "temperature must be finite"
            );
        }

        if (onlineValue != 0 && onlineValue != 1) {
            throw std::invalid_argument(
                "online status must be 0 or 1"
            );
        }

        if (sequence <= 0) {
            throw std::invalid_argument(
                "sequence must be positive"
            );
        }
// Update the existing device and save it safely.
bool updated = false;
bool saved = false;

{
    std::lock_guard<std::mutex> lock(devicesMutex);

    // Keep a copy in case saving fails.
    std::vector<Device> previousDevices = devices;

    updated = updateDeviceTelemetry(
        devices,
        deviceId,
        temperature,
        onlineValue == 1,
        sequence
    );

    if (updated) {
        saved = saveDevicesToCsv(
            devices,
            "devices.csv"
        );

        // Roll back the in-memory state if saving fails.
        if (!saved) {
            devices = previousDevices;
        }
    }
}

if (!updated) {
    std::cerr
        << "Device not found or stale sequence for device: "
        << deviceId << '\n';

    sendAll(
        clientFd,
        "ERROR: device not found or stale sequence\n"
    );
    return;
}

if (!saved) {
    std::cerr << "Failed to save device data\n";

    sendAll(
        clientFd,
        "ERROR: failed to save device data\n"
    );
    return;
}

std::cout << "[INFO] Device state updated and saved.\n";
        std::cout << "\nTelemetry data validated successfully\n";
        std::cout << "Device ID: " << deviceId << '\n';
        std::cout << "Temperature: " << temperature << " C\n";
        std::cout << "Online: "
                  << (onlineValue == 1 ? "Yes" : "No") << '\n';
        std::cout << "Sequence: " << sequence << '\n';

        // Apply the temperature alarm rule.
if (onlineValue == 1 && temperature > 80.0) {
    std::cerr << "[ALARM] High temperature detected!\n";
    std::cerr << "Device ID: " << deviceId << '\n';
    std::cerr << "Temperature: " << temperature << " C\n";
} else if (temperature > 80.0) {
    std::cout << "[INFO] Device is offline; "
                 "no active alarm.\n";
} else {
    std::cout << "[INFO] Temperature is normal.\n";
}
        // Send an acknowledgement to the client.
        sendAll(clientFd, "ACK from telemetry server\n");

    } catch (const std::exception& e) {
        std::cerr << "Invalid telemetry data: "
                  << e.what() << '\n';

        sendAll(clientFd, "ERROR: invalid telemetry data\n");
    }
}

void clientWorker(
    int clientFd,
    std::vector<Device>& devices,
    std::mutex& devicesMutex
) {
    // Set a receive timeout for this client.
timeval timeout{};
timeout.tv_sec = 5;
timeout.tv_usec = 0;

if (setsockopt(
        clientFd,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &timeout,
        sizeof(timeout)
    ) == -1) {
    std::perror("setsockopt SO_RCVTIMEO");
    close(clientFd);
    return;
}
    std::cout << "\nClient connected\n";

    handleClient(
        clientFd,
        devices,
        devicesMutex
    );

    close(clientFd);

    std::cout << "Client disconnected\n";
}

int main() {
    std::vector<Device> devices;

if (!loadDevicesFromCsv("devices.csv", devices)) {
    std::cerr << "Server could not load devices.csv\n";
    return 1;
}
std::mutex devicesMutex;
std::cout << "Loaded " << devices.size()
          << " device(s) from devices.csv\n";

    // 1. Create the listening socket.
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFd == -1) {
        std::perror("socket");
        return 1;
    }

    // 2. Allow the port to be reused after restarting the server.
    int opt = 1;

    if (setsockopt(
            serverFd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt,
            sizeof(opt)
        ) == -1) {
        std::perror("setsockopt");
        close(serverFd);
        return 1;
    }

    // 3. Configure the local server address.
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5000);

    if (inet_pton(
            AF_INET,
            "127.0.0.1",
            &serverAddr.sin_addr
        ) != 1) {
        std::cerr << "Invalid server IP address\n";
        close(serverFd);
        return 1;
    }

    // 4. Bind the socket to the address and port.
    if (bind(
            serverFd,
            reinterpret_cast<sockaddr*>(&serverAddr),
            sizeof(serverAddr)
        ) == -1) {
        std::perror("bind");
        close(serverFd);
        return 1;
    }

    // 5. Start listening for incoming connections.
    if (listen(serverFd, 10) == -1) {
        std::perror("listen");
        close(serverFd);
        return 1;
    }

    std::cout
        << "Telemetry server listening on 127.0.0.1:5000\n";

    // 6. Keep accepting clients until the server is stopped.
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);

        int clientFd = accept(
            serverFd,
            reinterpret_cast<sockaddr*>(&clientAddr),
            &clientLen
        );

        if (clientFd == -1) {
            std::perror("accept");
            continue;
        }
try {
    std::thread worker(
        clientWorker,
        clientFd,
        std::ref(devices),
        std::ref(devicesMutex)
    );

    worker.detach();

} catch (const std::exception& e) {
    std::cerr << "Failed to start client thread: "
              << e.what() << '\n';

    close(clientFd);
}

    }

    close(serverFd);
    return 0;
}
