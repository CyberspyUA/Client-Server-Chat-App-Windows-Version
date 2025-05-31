

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 *
 * @author Nikita Struk
 * @date May 30, 2025
 */

void InitializeServer() {
	WSADATA wsaData; // Winsock data structure
	int serverFD, newSocket, clientSockets[MAX_CLIENTS]; // Array to hold client sockets
	struct sockaddr_in address; // Structure to hold server address information
	int opt = 1, maxSD, activity; // Variables for select() and activity checking
	fd_set readfds; // File descriptor set for select()
	char buffer[BUFFER_SIZE + 1] = { 0 }; // Buffer for incoming messages
	int addrlen = sizeof(address); // Length of the address structure

	// Initialize Winsock
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
				if (valueRead <= 0) {
					getpeername(s, (struct sockaddr*)&address, &addrlen);
					printf("Client disconnected, socket fd is %d, client index is %d\n", s, clientIndex);
					closesocket(s);
					clientSockets[clientIndex] = 0;
				}
				else {
					buffer[valueRead] = '\0';
					printf("Client %d: %s\n", clientIndex, buffer);
					for (int i = 0; i < MAX_CLIENTS; i++) {
						if (clientSockets[i] > 0 && i != clientIndex) {
							send(clientSockets[i], buffer, valueRead, 0);
						}
					}
				}
			}
		}
	}

	closesocket(serverFD);
	WSACleanup();
}