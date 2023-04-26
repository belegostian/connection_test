#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;

    // Initialize Winsock
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    // Create a socket
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Set up server address and bind the socket
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(12345); // Use any port number you like
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    result = bind(listenSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
    if (result == SOCKET_ERROR) {
        std::cerr << "Error binding socket: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    result = listen(listenSocket, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        std::cerr << "Error listening on socket: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is listening for connections..." << std::endl;

    // Accept a client connection
    sockaddr_in clientAddress;
    int clientAddressSize = sizeof(clientAddress);
    SOCKET clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddress, &clientAddressSize);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Error accepting connection: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Client connected!" << std::endl;
    closesocket(listenSocket); // Close the listening socket

    // Receive and echo messages
    char buffer[1024];
    int bytesReceived;

    for (size_t i = 0; i < 1000; ++i) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            int bytesSent = send(clientSocket, buffer, bytesReceived, 0);
            if (bytesSent == SOCKET_ERROR) {
                std::cerr << "Error sending data: " << WSAGetLastError() << std::endl;
                break;
            }
        } else {
            std::cerr << "Error receiving data: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    // Close the client and listening sockets and clean up
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
