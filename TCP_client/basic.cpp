#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <chrono>
#include <vector>
#include <cmath>

#pragma comment(lib, "ws2_32.lib")

void evaluateQoS(const std::vector<double>& rtts);

int main() {
    WSADATA wsaData;

    // Initialize Winsock
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    // Create a socket
    SOCKET connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connectSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Set up server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(12345); // Use any port number you like
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr); // Use the server's IP address

    // Connect to the server
    result = connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
    if (result == SOCKET_ERROR) {
        std::cerr << "Error connecting to server: " << WSAGetLastError() << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    std::vector<double> rtts;
    size_t messageCount = 1000;
    size_t messageSize = 1024;
    char buffer[1024];
    std::string message(messageSize, 'hello, there');

    // Send and receive data
    for (size_t i = 0; i < messageCount; ++i) {
        auto start = std::chrono::steady_clock::now();
        result = send(connectSocket, message.c_str(), message.size(), 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "Error sending data: " << WSAGetLastError() << std::endl;
            break;
        }

        result = recv(connectSocket, buffer, sizeof(buffer), 0);
        if (result > 0) {
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            rtts.push_back(elapsed.count());
        } else {
            std::cerr << "Error receiving data: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    // Close the socket and clean up
    closesocket(connectSocket);
    WSACleanup();

    // Evaluate QoS
    evaluateQoS(rtts);

    return 0;
}


void evaluateQoS(const std::vector<double>& rtts) {
    if (rtts.empty()) {
        std::cerr << "No round trip times recorded." << std::endl;
        return;
    }

    double sum = 0;
    double sq_sum = 0;
    size_t received = rtts.size();

    for (const auto& rtt : rtts) {
        sum += rtt;
        sq_sum += rtt * rtt;
    }

    double mean = sum / received;
    double variance = (sq_sum / received) - (mean * mean);
    double stddev = std::sqrt(variance);
    double packetLossRate = 1.0 - (static_cast<double>(received) / 1000);
    double totalTime = sum;

    std::cout << "QoS Metrics:\n";
    std::cout << "1. Average Round Trip Time: " << mean << " seconds\n";
    std::cout << "2. Standard Deviation of Round Trip Time: " << stddev << " seconds\n";
    std::cout << "3. Packet Loss Rate: " << packetLossRate * 100 << "%\n";
    std::cout << "4. Total Time Spent: " << totalTime << " seconds\n";
}