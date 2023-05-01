#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>
#include <vector>
#include <thread>
#include <csignal>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

// TCPClient class to handle client-side TCP connection
class TCPClient
{
public:
    // Constructor: Takes IP address and port as parameters
    TCPClient(const std::string &ipAddress, unsigned short port);

    // Destructor: Cleans up Winsock resources
    ~TCPClient();

    // Establishes a connection to the server
    bool connectToServer();

    // Sends .nc file to server and  waits for response
    void sendFile(const std::string &filePath);

    // Closes the connection
    void closeConnection();

private:
    std::string ipAddress;
    unsigned short port;
    SOCKET connectSocket;
    WSADATA wsaData;
};

// Signal handler to catch interrupt signals
volatile sig_atomic_t interrupted = false;
void signalHandler(int signum)
{
    interrupted = true;
}

int main()
{
    // Set up signal handler
    std::signal(SIGINT, signalHandler);

    // Create a TCPClient instance with IP address and port number
    TCPClient client("127.0.0.1", 12345);
    std::string filePath = "C:/Users/Ian/Desktop/5axis_cut.nc";

    // Continuously send file until interrupted
    while (!interrupted)
    {   
        // Connect to the server
        if (client.connectToServer())
        {
            // Send file to server and wait for response
            client.sendFile(filePath);

            // Close the connection
            client.closeConnection();
        }

        std::cout << "Press Enter to send the file again or Ctrl+C to exit." << std::endl;

        // Wait for Enter key press
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }    

    return 0;
}


// Constructor implementation
TCPClient::TCPClient(const std::string &ipAddress, unsigned short port)
    : ipAddress(ipAddress), port(port), connectSocket(INVALID_SOCKET)
{
    // Initialize Winsock
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        std::exit(1);
    }
}

// Destructor implementation
TCPClient::~TCPClient()
{
    WSACleanup();
}

// Connects to the server with the specified IP address and port number
bool TCPClient::connectToServer()
{
    // Create a socket
    connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connectSocket == INVALID_SOCKET)
    {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // Set up server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &serverAddress.sin_addr);

    // Connect to the server
    int result = connect(connectSocket, (SOCKADDR *)&serverAddress, sizeof(serverAddress));
    if (result == SOCKET_ERROR)
    {
        std::cerr << "Error connecting to server: " << WSAGetLastError() << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return false;
    }

    return true;
}

// Sends the file to the server
void TCPClient::sendFile(const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        std::cerr << "Error opening file: " << filePath << std::endl;
        return;
    }

    const size_t bufferSize = 4096;
    std::vector<char> buffer(bufferSize);
    std::streamsize bytesRead;

    auto sendAll = [&](const char *data, size_t len)
    {
        int totalSent = 0;
        while (totalSent < len)
        {
            int bytesSent = send(connectSocket, data + totalSent, len - totalSent, 0);
            if (bytesSent == SOCKET_ERROR)
            {
                return SOCKET_ERROR;
            }
            totalSent += bytesSent;
        }
        return totalSent;
    };

    // Calculate file size
    file.seekg(0, std::ios::end);
    int64_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    for (size_t i = 0; i < 1; i++)
    {
        // Send file size before sending file data
        if (sendAll(reinterpret_cast<const char *>(&fileSize), sizeof(fileSize)) == SOCKET_ERROR)
        {
            std::cerr << "Error sending file size: " << WSAGetLastError() << std::endl;
            break;
        }

        // Reset the file pointer to the beginning
        file.seekg(0, std::ios::beg);

        while ((bytesRead = file.readsome(buffer.data(), bufferSize)) > 0)
        {
            if (sendAll(buffer.data(), bytesRead) == SOCKET_ERROR)
            {
                std::cerr << "Error sending data: " << WSAGetLastError() << std::endl;
                break;
            }
        }

        // Clear EOF flag
        file.clear();

        std::vector<char> response(1024);
        int responseSize = recv(connectSocket, response.data(), response.size(), 0);
        if (responseSize > 0)
        {
            std::string serverResponse(response.data(), responseSize);
            std::cout << serverResponse << std::endl;
        }
        else
        {
            std::cerr << "Error receiving response from server: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    file.close(); // Close the file after the loop is finished
}

// Closes the connection by closing the socket
void TCPClient::closeConnection()
{
    closesocket(connectSocket);
}