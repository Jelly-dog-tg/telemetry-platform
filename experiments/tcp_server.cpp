#include "CsvStorage.h"
#include "Device.h"
#include "DeviceManager.h"
#include "ProtocolMessage.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <exception>
#include <functional>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <vector>
#include <atomic>

struct ServerStatistics {

    std::atomic<int> connections{0};

    std::atomic<int> messagesReceived{0};

    std::atomic<int> successfulMessages{0};

    std::atomic<int> failedMessages{0};
};
// Send the entire response, handling partial sends.
bool sendAll(int socketFd, const std::string& data) {
    std::size_t totalSent = 0;

    while (totalSent < data.size()) {
        ssize_t sent = send(
            socketFd,
            data.data() + totalSent,
            data.size() - totalSent,
            MSG_NOSIGNAL
        );

        if (sent == -1) {
            if (errno == EINTR) {
                continue;
            }

            std::perror("send");
            return false;
        }

        if (sent == 0) {
            std::cerr << "Connection closed during send\n";
            return false;
        }

        totalSent += static_cast<std::size_t>(sent);
    }

    return true;
}

// Receive one newline-terminated message.
// Keep unprocessed bytes for the next message.
enum class ReceiveResult {
    Message,
    Closed,
    Error
};

ReceiveResult receiveLine(
    int clientFd,
    std::string& pending,
    std::string& line
) {
    constexpr std::size_t maxMessageSize = 1024;
    char chunk[256];

    line.clear();

    while (true) {
        const std::size_t pos = pending.find('\n');

        if (pos != std::string::npos) {
            if (pos + 1 > maxMessageSize) {
                std::cerr << "Message is too large\n";
                return ReceiveResult::Error;
            }

            line = pending.substr(0, pos + 1);
            pending.erase(0, pos + 1);
            return ReceiveResult::Message;
        }

        if (pending.size() >= maxMessageSize) {
            std::cerr << "Message exceeds size limit\n";
            return ReceiveResult::Error;
        }

        const ssize_t received = recv(
            clientFd, chunk, sizeof(chunk), 0
        );

        if (received > 0) {
            pending.append(
                chunk,
                static_cast<std::size_t>(received)
            );
            continue;
        }

        if (received == 0) {
            if (!pending.empty()) {
                std::cerr << "Incomplete message on disconnect\n";
                return ReceiveResult::Error;
            }

            return ReceiveResult::Closed;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            std::cerr << "Client receive timed out\n";
        } else {
            std::perror("recv");
        }

        return ReceiveResult::Error;
    }
}

// Process one registered-device operation or telemetry message.
bool handleClient(
    int clientFd,
    std::vector<Device>& devices,
    std::mutex& devicesMutex,
    const std::string& buffer
) {
    ProtocolMessage message;
    std::string parseError;

    if (!parseProtocolMessage(
            buffer,
            message,
            parseError
        )) {
        std::cerr << "Invalid protocol message: "
                  << parseError << '\n';

        sendAll(clientFd, "ERROR: " + parseError + "\n");
        return false;
    }

    // --------------------------------------------------------
    // 1. REGISTER: register a new device.
    // --------------------------------------------------------
    if (message.type == MessageType::Register) {
        bool alreadyRegistered = false;
        bool added = false;
        bool saved = false;

        {
            std::lock_guard<std::mutex> lock(devicesMutex);

            alreadyRegistered = deviceExists(
                devices,
                message.deviceId
            );

            if (!alreadyRegistered) {
                const std::vector<Device> previousDevices = devices;

                added = addDevice(
                    devices,
                    Device{
                        message.deviceId,
                        0.0,
                        true,
                        0
                    }
                );

                if (added) {
                    saved = saveDevicesToCsv(
                        devices,
                        "devices.csv"
                    );

                    if (!saved) {
                        devices = previousDevices;
                    }
                }
            }
        }

        if (alreadyRegistered) {
            return sendAll(clientFd, "ACK: already registered\n");
        }

        if (!added) {
            sendAll(clientFd, "ERROR: registration failed\n");
        return false;
        }

        if (!saved) {
            sendAll(clientFd, "ERROR: failed to save device\n");
        return false;
        }

        std::cout << "[INFO] Device registered: "
                  << message.deviceId << '\n';

        return sendAll(clientFd, "ACK: registered\n");
    }

    // --------------------------------------------------------
    // 2. HEARTBEAT: update device online state and sequence.
    // --------------------------------------------------------
    if (message.type == MessageType::Heartbeat) {
        bool updated = false;
        bool saved = false;

        {
            std::lock_guard<std::mutex> lock(devicesMutex);

            const std::vector<Device> previousDevices = devices;

            updated = updateDeviceHeartbeat(
                devices,
                message.deviceId,
                message.sequence
            );

            if (updated) {
                saved = saveDevicesToCsv(
                    devices,
                    "devices.csv"
                );

                if (!saved) {
                    devices = previousDevices;
                }
            }
        }

        if (!updated) {
            sendAll(clientFd, "ERROR: unknown device or stale sequence\n");
        return false;
        }

        if (!saved) {
            sendAll(clientFd, "ERROR: failed to save device\n");
        return false;
        }

        std::cout << "[INFO] Heartbeat received from device "
                  << message.deviceId << '\n';

        return sendAll(clientFd, "ACK: heartbeat\n");
    }

    // --------------------------------------------------------
    // 3. TELEMETRY: update an existing device's telemetry.
    // --------------------------------------------------------
    const int deviceId = message.deviceId;
    const double temperature = message.temperature;
    const bool online = message.online;
    const int sequence = message.sequence;

    bool updated = false;
    bool saved = false;

    {
        std::lock_guard<std::mutex> lock(devicesMutex);

        const std::vector<Device> previousDevices = devices;

        updated = updateDeviceTelemetry(
            devices,
            deviceId,
            temperature,
            online,
            sequence
        );

        if (updated) {
            saved = saveDevicesToCsv(
                devices,
                "devices.csv"
            );

            if (!saved) {
                devices = previousDevices;
            }
        }
    }

    if (!updated) {
        std::cerr
            << "Device not found or stale sequence for device: "
            << deviceId << '\n';

        sendAll(clientFd, "ERROR: device not found or stale sequence\n");
        return false;
    }

    if (!saved) {
        std::cerr << "Failed to save device data\n";

        sendAll(clientFd, "ERROR: failed to save device data\n");
        return false;
    }

    std::cout << "\nTelemetry data validated successfully\n";
    std::cout << "Device ID: " << deviceId << '\n';
    std::cout << "Temperature: " << temperature << " C\n";
    std::cout << "Online: "
              << (online ? "Yes" : "No") << '\n';
    std::cout << "Sequence: " << sequence << '\n';
    std::cout << "[INFO] Device state updated and saved.\n";

    if (online && temperature > 80.0) {
        std::cerr << "[ALARM] High temperature detected!\n";
        std::cerr << "Device ID: " << deviceId << '\n';
        std::cerr << "Temperature: " << temperature << " C\n";
    } else if (temperature > 80.0) {
        std::cout
            << "[INFO] Device is offline; no active alarm.\n";
    } else {
        std::cout << "[INFO] Temperature is normal.\n";
    }

    return sendAll(
        clientFd,
        "ACK from telemetry server\n"
    );
}
// Periodically check heartbeat timeout.
void heartbeatMonitor(
    std::vector<Device>& devices,
    std::mutex& devicesMutex,
    ServerStatistics& statistics
) {
    constexpr int timeoutSeconds = 30;

    while (true) {

        std::this_thread::sleep_for(
            std::chrono::seconds(5)
        );

        bool changed = false;

        {
            std::lock_guard<std::mutex> lock(
                devicesMutex
            );

            const std::vector<Device> previousDevices =
                devices;


            checkHeartbeatTimeout(
                devices,
                timeoutSeconds
            );


            for (std::size_t i = 0;
                 i < devices.size();
                 ++i) {

                if (devices[i].online !=
                    previousDevices[i].online) {

                    changed = true;
                    break;
                }
            }


            if (changed) {

                if (!saveDevicesToCsv(
                        devices,
                        "devices.csv")) {

                    std::cerr
                        << "[ERROR] Failed to save "
                        << "heartbeat timeout state\n";

                    devices = previousDevices;
                }
            }
        }

        int onlineCount = 0;

        {
            std::lock_guard<std::mutex> lock(devicesMutex);
            onlineCount = countOnlineDevices(devices);
        }

        std::cout
            << "\n========== Server Statistics ==========\n"
            << "Total connections: "
            << statistics.connections.load()
            << "\nMessages received: "
            << statistics.messagesReceived.load()
            << "\nSuccessful messages: "
            << statistics.successfulMessages.load()
            << "\nFailed messages: "
            << statistics.failedMessages.load()
            << "\nOnline devices: "
            << onlineCount
            << "\n=======================================\n";
    }
}

// Handle the lifetime of one client connection.
void clientWorker(
    int clientFd,
    std::vector<Device>& devices,
    std::mutex& devicesMutex,
    ServerStatistics& statistics
) {
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
    statistics.connections++;
    try {
        std::string pending;

        while (true) {
            std::string line;

            const ReceiveResult result = receiveLine(
                clientFd, pending, line
            );

            if (result == ReceiveResult::Closed) {
                break;
            }

            if (result == ReceiveResult::Error) {
                sendAll(
                    clientFd,
                    "ERROR: incomplete, oversized or timed-out message\n"
                );
                break;
            }

statistics.messagesReceived++;

            try {
                const bool success = handleClient(
                    clientFd,
                    devices,
                    devicesMutex,
                    line
                );

                if (success) {
                    statistics.successfulMessages++;
                } else {
                    statistics.failedMessages++;
                }

            } catch (...) {
                statistics.failedMessages++;
                throw;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Client processing exception: "
                  << e.what() << '\n';

        sendAll(
            clientFd,
            "ERROR: internal server error\n"
        );
    } catch (...) {
        std::cerr << "Unknown client processing exception\n";

        sendAll(
            clientFd,
            "ERROR: internal server error\n"
        );
    }

    close(clientFd);

    std::cout << "Client disconnected\n";
}

int main() {
    // Load device state before accepting connections.
    std::vector<Device> devices;

    if (!loadDevicesFromCsv("devices.csv", devices)) {
        std::cerr << "Server could not load devices.csv\n";
        return 1;
    }

    std::mutex devicesMutex;
ServerStatistics statistics;    
std::thread monitorThread(
    heartbeatMonitor,
    std::ref(devices),
    std::ref(devicesMutex),
    std::ref(statistics)
);

monitorThread.detach();
    std::cout << "Loaded "
              << devices.size()
              << " device(s) from devices.csv\n";

    // Create the listening socket.
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFd == -1) {
        std::perror("socket");
        return 1;
    }

    // Allow reuse of the local port after restarting.
    int opt = 1;

    if (setsockopt(
            serverFd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt,
            sizeof(opt)
        ) == -1) {
        std::perror("setsockopt SO_REUSEADDR");
        close(serverFd);
        return 1;
    }

    // Configure the server address.
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

    if (bind(
            serverFd,
            reinterpret_cast<sockaddr*>(&serverAddr),
            sizeof(serverAddr)
        ) == -1) {
        std::perror("bind");
        close(serverFd);
        return 1;
    }

    if (listen(serverFd, 10) == -1) {
        std::perror("listen");
        close(serverFd);
        return 1;
    }

    std::cout
        << "Telemetry server listening on 127.0.0.1:5000\n";

    // Accept clients continuously. Each client gets its own thread.
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);

        int clientFd = accept(
            serverFd,
            reinterpret_cast<sockaddr*>(&clientAddr),
            &clientLen
        );

        if (clientFd == -1) {
            if (errno == EINTR) {
                continue;
            }

            std::perror("accept");
            continue;
        }

        try {
            std::thread worker(
                clientWorker,
                clientFd,
                std::ref(devices),
                std::ref(devicesMutex),
                std::ref(statistics)
            );

            worker.detach();
        } catch (const std::exception& e) {
            std::cerr << "Failed to start client thread: "
                      << e.what() << '\n';

            close(clientFd);
        }
    }

    // The current server runs until externally terminated.
    close(serverFd);
    return 0;
}
