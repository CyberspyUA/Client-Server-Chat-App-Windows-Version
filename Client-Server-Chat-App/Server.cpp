

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <map>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

/**
 * @file Server.cpp
 * @brief Multi-client chat server using Windows Sockets API.
 *
 * This server listens for incoming TCP connections on a specified port (8080).
 * It accepts up to MAX_CLIENTS simultaneous clients, relays messages between them,
 * and handles client disconnections. The implementation uses the Winsock2 API
 * and is intended for Windows platforms only.
 *
 * Key features:
 * - Accepts multiple client connections using select() for multiplexing.
 * - Broadcasts received messages to all connected clients except the sender.
 * - Cleans up resources and handles errors gracefully.
 * - Logs messages to a file with timestamps.
 *
 * @author Nikita Struk
 * @date May 30, 2025
 * Last updated: June 3, 2025
 */

void LogMessage(const char* message) 
{
	std::ofstream logFile("server.log", std::ios::app);
	if (logFile.is_open()) 
	{
		// Get current time as system time
		auto now = std::chrono::system_clock::now();

		// Convert system time to time_t
		std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

		// Convert time_t to tm structure
		std::tm localTime;
		localtime_s(&localTime, &currentTime); // Use localtime_s for thread safety

		// Log the message with timestamp
		logFile << std::put_time(&localTime, "(%m/%d/%H:%M)") << " " << message << std::endl;
		logFile.close();
	} 
	else 
	{
		printf("Could not open log file.\n");
	}
}

// Helper to trim whitespace

std::string trim(const std::string& s) {

	size_t start = s.find_first_not_of(" \t\r\n");

	size_t end = s.find_last_not_of(" \t\r\n");

	return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);

}



void ServerConsoleThread(std::map<std::string, int>& nameToSocket, int clientSockets[], std::mutex& mapMutex) {

	while (true) {

		std::string input;

		std::getline(std::cin, input);

		input = trim(input);

		if (input.rfind("/kick ", 0) == 0) 
		{
			std::string clientName = trim(input.substr(6));
			std::lock_guard<std::mutex> lock(mapMutex);
			auto it = nameToSocket.find(clientName);
			if (it != nameToSocket.end()) 
			{
				int sock = it->second;
				send(sock, "You have been kicked by the server.", 35, 0);
				closesocket(sock);
				// Remove from clientSockets array
				for (int i = 0; i < MAX_CLIENTS; ++i) 
				{
					if (clientSockets[i] == sock) 
					{
						clientSockets[i] = 0;
						break;
					}
				}
				nameToSocket.erase(it);
				printf("Client '%s' has been kicked.\n", clientName.c_str());
			}
			else 
			{
				printf("No client with nickname '%s' found.\n", clientName.c_str());
			}
		}
	}
}

void InitializeServer() {
	WSADATA wsaData; // Winsock data structure
	int serverFD, newSocket, clientSockets[MAX_CLIENTS]; // Array to hold client sockets
	struct sockaddr_in address; // Structure to hold server address information
	int opt = 1, maxSD, activity; // Variables for select() and activity checking
	fd_set readfds; // File descriptor set for select()
	char buffer[BUFFER_SIZE + 1] = { 0 }; // Buffer for incoming messages
	int addrlen = sizeof(address); // Length of the address structure

	// Map nickname to socket
	std::map<std::string, int> nameToSocket;
	std::mutex mapMutex;
	// Start server console thread for /kick command
	std::thread consoleThread(ServerConsoleThread, std::ref(nameToSocket), clientSockets, std::ref(mapMutex));
	consoleThread.detach();
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
	{
		printf("WSAStartup failed\n");
		exit(EXIT_FAILURE);
	}

	// Initialize client sockets to 0
	for (int clientIndex = 0; clientIndex < MAX_CLIENTS; clientIndex++) clientSockets[clientIndex] = 0;
	// Create a socket for the server
	if ((serverFD = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) 
	{
		printf("Socket creation failed: %d\n", WSAGetLastError());
		WSACleanup();
		exit(EXIT_FAILURE);
	}
	// Set socket options to allow reuse of the address
	if (setsockopt(serverFD, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) 
	{
		printf("setsockopt failed: %d\n", WSAGetLastError());
		closesocket(serverFD);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	// Bind the socket to the specified port
	if (bind(serverFD, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) 
	{
		printf("Bind failed: %d\n", WSAGetLastError());
		closesocket(serverFD);
		WSACleanup();
		exit(EXIT_FAILURE);
	}
	// Start listening for incoming connections
	if (listen(serverFD, 3) == SOCKET_ERROR) 
	{
		printf("Listen failed: %d\n", WSAGetLastError());
		closesocket(serverFD);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	printf("Server listening on port %d...\n", PORT);

	while (1) 
	{
		FD_ZERO(&readfds);
		FD_SET(serverFD, &readfds);
		maxSD = serverFD;
		// Add client sockets to the set
		for (int clientIndex = 0; clientIndex < MAX_CLIENTS; clientIndex++) 
		{
			if (clientSockets[clientIndex] > 0)
				FD_SET(clientSockets[clientIndex], &readfds);
			if (clientSockets[clientIndex] > maxSD)
				maxSD = clientSockets[clientIndex];
		}

		activity = select(0, &readfds, NULL, NULL, NULL); // 0 for nfds on Windows
		// Check for errors in select
		if (activity == SOCKET_ERROR) 
		{
			printf("Select error: %d\n", WSAGetLastError());
			break;
		}
		// Check if there is an incoming connection on the server socket
		if (FD_ISSET(serverFD, &readfds)) {
			if ((newSocket = accept(serverFD, (struct sockaddr*)&address, &addrlen)) == INVALID_SOCKET) 
			{
				printf("Accept failed: %d\n", WSAGetLastError());
				break;
			}

			for (int clientIndex = 0; clientIndex < MAX_CLIENTS; clientIndex++) 
			{
				if (clientSockets[clientIndex] == 0) {
					clientSockets[clientIndex] = newSocket;
					printf("New connection, socket fd is %d, client index is %d\n", newSocket, clientIndex);
					break;
				}
			}
		}
		// Check for incoming messages from clients
		for (int clientIndex = 0; clientIndex < MAX_CLIENTS; clientIndex++) 
		{
			SOCKET s = clientSockets[clientIndex];
			if (s > 0 && FD_ISSET(s, &readfds)) {
				int valueRead = recv(s, buffer, BUFFER_SIZE, 0);
				if (valueRead <= 0) 
				{
					getpeername(s, (struct sockaddr*)&address, &addrlen);
					printf("Client disconnected, socket fd is %d, client index is %d\n", s, clientIndex);
					closesocket(s);
					clientSockets[clientIndex] = 0;
				}
				else 
				{
					buffer[valueRead] = '\0';
					printf("%s\n", buffer);
					LogMessage(buffer);
					// Extract nickname (format: "nickname: message")
					std::string msg(buffer);
					size_t sep = msg.find(": ");
					if (sep != std::string::npos) 
					{
						std::string nickname = msg.substr(0, sep);
						std::lock_guard<std::mutex> lock(mapMutex);
						nameToSocket[nickname] = s;
					}
					for (int i = 0; i < MAX_CLIENTS; i++) 
					{
						if (clientSockets[i] > 0 && i != clientIndex) 
						{
							send(clientSockets[i], buffer, valueRead, 0);
							LogMessage(buffer); // Log the message to server.log
						}
					}
				}
			}
		}
	}

	closesocket(serverFD);
	WSACleanup();
}