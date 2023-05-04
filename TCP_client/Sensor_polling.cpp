#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <chrono>
#include <vector>
#include <cmath>

#pragma comment(lib, "ws2_32.lib")

// TCPClient class to handle client-side TCP connection
class TCPClient
{
public:
    // Constructor: Takes IP address and port as parameters [1]
    TCPClient(const std::string &ipAddress, unsigned short port)
        : ipAddress(ipAddress), port(port), connectSocket(INVALID_SOCKET)
    {
        // Initialize Winsock [2]
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            std::cerr << "WSAStartup failed: " << result << std::endl;
            std::exit(1);
        }
    }

    // Destructor: Cleans up Winsock resources [3]
    ~TCPClient()
    {
        WSACleanup();
    }

    // Establishes a connection to the server
    bool connectToServer();

    // Sends and receives messages with the server
    void sendAndReceiveMessages(size_t messageCount, size_t messageSize);

    // Closes the connection
    void closeConnection();

    // Evaluates Quality of Service (QoS) metrics
    void evaluateQoS(const std::vector<double> &rtts);

//[4]
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
        // Send and receive messages
        client.sendAndReceiveMessages(1000, 1024);

        // Close the connection
        client.closeConnection();
    }

    return 0;
}

// Connects to the server with the specified IP address and port number
bool TCPClient::connectToServer()
{
    // Create a socket [5]
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
		// [6]
    inet_pton(AF_INET, ipAddress.c_str(), &serverAddress.sin_addr);

    // Connect to the server [7]
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

// Sends and receives messages with the server, and evaluates QoS metrics
void TCPClient::sendAndReceiveMessages(size_t messageCount, size_t messageSize)
{
    std::vector<double> rtts;
    char buffer[1024];
    std::string message(messageSize, 'Hello, there! ');

    // Send and receive data
    for (size_t i = 0; i < messageCount; ++i)
    {
				// [8]
        auto start = std::chrono::steady_clock::now();
        int result = send(connectSocket, message.c_str(), message.size(), 0);
        if (result == SOCKET_ERROR)
        {
            std::cerr << "Error sending data: " << WSAGetLastError() << std::endl;
            break;
        }
				// [9]
        result = recv(connectSocket, buffer, sizeof(buffer), 0);
        if (result > 0)
        {
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = end - start;
						// [10]
            rtts.push_back(elapsed.count());
        }
        else
        {
            std::cerr << "Error receiving data: " << WSAGetLastError() << std::endl;
						// [11]
            break;
        }
    }

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

    // Calculate sum and squared sum of RTTs [12]
    for (const auto &rtt : rtts)
    {
        sum += rtt;
        sq_sum += rtt * rtt;
    }

    // Calculate mean, variance, and standard deviation of RTTs
    double mean = sum / received;
		// [13]
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