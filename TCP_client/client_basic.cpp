#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>
#include <vector>
#include <cmath>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

// TCPClient class to handle client-side TCP connection
class TCPClient
{
public:
    // Constructor: Takes IP address and port as parameters
    TCPClient(const std::string &ipAddress, unsigned short port)
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

    // Destructor: Cleans up Winsock resources
    ~TCPClient()
    {
        WSACleanup();
    }

    // Establishes a connection to the server
    bool connectToServer();

    // Sends .nc file to server and  waits for response
    void sendFile(const std::string &filePath);

    // Closes the connection
    void closeConnection();

    // Evaluates Quality of Service (QoS) metrics
    void evaluateQoS(const std::vector<double> &rtts);

private:
    std::string ipAddress;
    unsigned short port;
    SOCKET connectSocket;
    WSADATA wsaData;
};

int main()
{
    // Create a TCPClient instance with IP address and port number
    TCPClient client("127.0.0.1", 12345);

    // Connect to the server
    if (client.connectToServer())
    {
        // Send file to server and wait for response
        std::string filePath = "C:/Users/Ian/Desktop/5axis_cut.nc";
        client.sendFile(filePath);

        // Close the connection
        client.closeConnection();
    }

    return 0;
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

void TCPClient::sendFile(const std::string &filePath)
{
    std::vector<double> rtts;
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        std::cerr << "Error opening file: " << filePath << std::endl;
        return;
    }

    const size_t bufferSize = 4096;
    std::vector<char> buffer(bufferSize);
    std::streamsize bytesRead;

    auto sendAll = [&](const char* data, size_t len) {
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
        if (sendAll(reinterpret_cast<const char*>(&fileSize), sizeof(fileSize)) == SOCKET_ERROR)
        {
            std::cerr << "Error sending file size: " << WSAGetLastError() << std::endl;
            break;
        }

        file.seekg(0, std::ios::beg); // Reset the file pointer to the beginning
        auto start = std::chrono::steady_clock::now();
        while ((bytesRead = file.readsome(buffer.data(), bufferSize)) > 0)
        {            
            if (sendAll(buffer.data(), bytesRead) == SOCKET_ERROR)
            {
                std::cerr << "Error sending data: " << WSAGetLastError() << std::endl;
                break;
            }
        }

        file.clear(); // Clear EOF flag

        std::vector<char> response(1024);
        int responseSize = recv(connectSocket, response.data(), response.size(), 0);
        if (responseSize > 0)
        {
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            rtts.push_back(elapsed.count());

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

    // Evaluate QoS
    evaluateQoS(rtts);
}

// Closes the connection by closing the socket
void TCPClient::closeConnection()
{
    closesocket(connectSocket);
}

// Evaluates QoS metrics based on recorded round-trip times (RTTs)
void TCPClient::evaluateQoS(const std::vector<double> &rtts)
{
    if (rtts.empty())
    {
        std::cerr << "No round trip times recorded." << std::endl;
        return;
    }
    double sum = 0;
    double sq_sum = 0;
    size_t received = rtts.size();

    // Calculate sum and squared sum of RTTs
    for (const auto &rtt : rtts)
    {
        sum += rtt;
        sq_sum += rtt * rtt;
    }

    // Calculate mean, variance, and standard deviation of RTTs
    double mean = sum / received;
    double variance = (sq_sum / received) - (mean * mean);
    double stddev = std::sqrt(variance);

    // Calculate packet loss rate and total time spent
    double packetLossRate = 1.0 - (static_cast<double>(received) / 1000);
    double totalTime = sum;

    // Display QoS metrics
    std::cout << "QoS Metrics:\n";
    std::cout << "1. Average Round Trip Time: " << mean << " seconds\n";
    std::cout << "2. Standard Deviation of Round Trip Time: " << stddev << " seconds\n";
    std::cout << "3. Packet Loss Rate: " << packetLossRate * 100 << "%\n";
    std::cout << "4. Total Time Spent: " << totalTime << " seconds\n";
}