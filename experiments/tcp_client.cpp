#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

int main() {
    // 1. Create a TCP socket.
    int clientFd = socket(AF_INET, SOCK_STREAM, 0);

    if (clientFd == -1) {
        std::perror("socket");
        return 1;
    }

    // 2. Configure the server address.
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5000);

    if (inet_pton(
            AF_INET,
            "127.0.0.1",
            &serverAddr.sin_addr
        ) != 1) {
        std::cerr << "Invalid server IP address\n";
        close(clientFd);
        return 1;
    }

    // 3. Connect to the server.
    if (connect(
            clientFd,
            reinterpret_cast<sockaddr*>(&serverAddr),
            sizeof(serverAddr)
        ) == -1) {
        std::perror("connect");
        close(clientFd);
        return 1;
    }

    std::cout << "Connected to server\n";

    // 4. Simulate one telemetry device.
    int deviceId = 1;
    double temperature = 36.5;
    bool online = true;
    int sequence = 4;

    // 5. Build the telemetry message.
    std::ostringstream messageStream;

    messageStream << "TELEMETRY,"
                  << deviceId << ','
                  << std::fixed << std::setprecision(1)
                  << temperature << ','
                  << (online ? 1 : 0) << ','
                  << sequence << '\n';

    std::string message = messageStream.str();

    std::cout << "Sending: " << message;

    // 6. Send the message to the server.
    ssize_t sent = send(
        clientFd,
        message.data(),
        message.size(),
        0
    );

    if (sent == -1) {
        std::perror("send");
        close(clientFd);
        return 1;
    }

    if (static_cast<std::size_t>(sent) != message.size()) {
        std::cerr << "Warning: message was not fully sent\n";
        close(clientFd);
        return 1;
    }

    // 7. Receive the server's reply.
    char buffer[1024];

    ssize_t received = recv(
        clientFd,
        buffer,
        sizeof(buffer) - 1,
        0
    );

    if (received == -1) {
        std::perror("recv");
        close(clientFd);
        return 1;
    }

    if (received > 0) {
        buffer[received] = '\0';
        std::cout << "Server replied: " << buffer;
    } else {
        std::cout << "Server closed the connection\n";
    }

    // 8. Close the socket.
    close(clientFd);

    return 0;
}
