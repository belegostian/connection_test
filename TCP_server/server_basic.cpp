#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

// TCPServer class that encapsulates the server logic
class TCPServer
{
public:
    // Constructor taking IP address and port number as parameters
    TCPServer(const std::string &ip, int port) : ip_(ip), port_(port) {}

    // Initialize the server
    bool init();

    // Start listening for connections and handle incoming data
    void run();

    // receive chunk of file, save it, than return successful status
    void saveFileAndSendResponse(SOCKET clientSocket);

private:
    // IP address of the server
    std::string ip_;
    // Port number of the server
    int port_;
    // Listening socket for incoming connections
    SOCKET listenSocket_;
};

int main()
{
    // Set IP address and port number
    std::string ip = "127.0.0.1";
    int port = 12345;

    // Create a TCPServer instance
    TCPServer server(ip, port);

    // Initialize the server
    if (server.init())
    {
        // Run the server
        server.run();
    }
    else
    {
        std::cerr << "Failed to initialize the server." << std::endl;
        return 1;
    }

    return 0;
}

bool TCPServer::init()
{
    WSADATA wsaData;
    // Initialize Winsock
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }

    // Create a socket
    listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket_ == INVALID_SOCKET)
    {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // Set up server address and bind the socket
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port_);
    inet_pton(AF_INET, ip_.c_str(), &serverAddress.sin_addr.s_addr);

    result = bind(listenSocket_, (SOCKADDR *)&serverAddress, sizeof(serverAddress));
    if (result == SOCKET_ERROR)
    {
        std::cerr << "Error binding socket: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket_);
        WSACleanup();
        return false;
    }

    return true;
}

void TCPServer::run()
{
    // Listen for incoming connections
    int result = listen(listenSocket_, SOMAXCONN);
    if (result == SOCKET_ERROR)
    {
        std::cerr << "Error listening on socket: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket_);
        WSACleanup();
        return;
    }

    std::cout << "Server is listening for connections..." << std::endl;

    // Accept a client connection
    sockaddr_in clientAddress;
    int clientAddressSize = sizeof(clientAddress);
    SOCKET clientSocket = accept(listenSocket_, (SOCKADDR *)&clientAddress, &clientAddressSize);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "Error accepting connection: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket_);
        WSACleanup();
        return;
    }

    std::cout << "Client connected!" << std::endl;
    // Close the listening socket
    closesocket(listenSocket_);

    // Save the received file and send a response
    saveFileAndSendResponse(clientSocket);

    // Close the client and listening sockets and clean up
    closesocket(clientSocket);
    WSACleanup();
}

void TCPServer::saveFileAndSendResponse(SOCKET clientSocket)
{
    static size_t version = 0;

    std::string folderPath = "C:/Users/Ian/Desktop/backup/";
    std::string fileName = "backup_" + std::to_string(++version) + ".nc";
    std::ofstream outputFile(folderPath + fileName, std::ios::binary);

    if (!outputFile)
    {
        std::cerr << "Error opening file: " << folderPath + fileName << std::endl;
        return;
    }

    const size_t bufferSize = 4096;
    std::vector<char> buffer(bufferSize);
    int64_t fileSize;

    // Read file size
    int received = recv(clientSocket, reinterpret_cast<char *>(&fileSize), sizeof(fileSize), 0);
    if (received != sizeof(fileSize))
    {
        std::cerr << "Error receiving file size" << std::endl;
        return;
    }

    int64_t bytesReceived = 0;

    while (bytesReceived < fileSize)
    {
        int bytesRead = recv(clientSocket, buffer.data(), bufferSize, 0);
        if (bytesRead > 0)
        {
            outputFile.write(buffer.data(), bytesRead);
            bytesReceived += bytesRead;
        }
        else if (bytesRead == 0)
        {
            std::cerr << "Client disconnected" << std::endl;
            break;
        }
        else
        {
            std::cerr << "Error receiving file data: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    outputFile.close();

    // Send a response to the client
    std::string response = "File received";
    send(clientSocket, response.data(), response.size(), 0);
}