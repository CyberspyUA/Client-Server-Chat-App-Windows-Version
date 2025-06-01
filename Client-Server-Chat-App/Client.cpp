#pragma once

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

/**
 * @file Client.cpp
 * @brief Simple Windows chat client using Winsock and multithreading.
 *
 * Connects to a TCP server (by default, on localhost port 8080), sends user input, and
 * receives messages from the server in a separate thread. Uses the Windows
 * Sockets API for networking and Windows threads for concurrent message
 * reception.
 *
 * Features:
 * - Establishes a connection to the server.
 * - Sends user-typed messages to the server.
 * - Receives and displays messages from the server asynchronously.
 *
 * @author Nikita Struk
 * @date May 30, 2025
 */

// Function to receive messages from the server in a separate thread
DWORD WINAPI receive_messages(LPVOID socket_desc) 
{
	int sock = *(int*)socket_desc;
	char buffer[BUFFER_SIZE];
	int valread;
	while (1) {
		memset(buffer, 0, BUFFER_SIZE);
		valread = recv(sock, buffer, BUFFER_SIZE - 1, 0);
		if (valread <= 0) break;
		buffer[valread] = '\0';
		printf("Received: %s\n", buffer);
	}
	return 0;
}

void InitializeClient(const std::string& serverAddress = "127.0.0.1", 
                      const unsigned int& serverPort = 8080, 
                      const std::string& userNickname = "Anonymous")
{  
    WSADATA wsaData;  
    int sock = 0;  
    struct sockaddr_in serv_addr;  
    char buffer[BUFFER_SIZE];  

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)  
    {  
        printf("WSAStartup failed\n");  
        exit(EXIT_FAILURE);  
    }  

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)  
    {  
        perror("Socket creation failed");  
        exit(EXIT_FAILURE);  
    }  

    serv_addr.sin_family = AF_INET;  
    serv_addr.sin_port = htons(serverPort);

    // Convert std::string to C-style string using c_str()  
    if (inet_pton(AF_INET, serverAddress.c_str(), &serv_addr.sin_addr) <= 0)  
    {  
        perror("Invalid address");  
        exit(EXIT_FAILURE);  
    }  
	// Connect to the server
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)  
    {  
        perror("Connection failed");  
        exit(EXIT_FAILURE);  
    }  

    printf("Connected to server\n");  
	// Create a thread to receive messages from the server
    HANDLE thread_handle = CreateThread(  
        NULL, 0, receive_messages, &sock, 0, NULL  
    );  
    if (thread_handle == NULL)  
    {  
        perror("Thread creation failed");  
        exit(EXIT_FAILURE);  
    }  
	// Set the thread to run in the background
    while (1)  
    {  
		printf("Enter message: ");
		fgets(buffer, BUFFER_SIZE, stdin);

		// Remove trailing newline from fgets
		buffer[strcspn(buffer, "\r\n")] = 0;

		// Convert buffer to lowercase for case-insensitive comparison
		char lower_buffer[BUFFER_SIZE];
		strncpy_s(lower_buffer, buffer, BUFFER_SIZE);
		lower_buffer[BUFFER_SIZE - 1] = '\0';
		for (size_t i = 0; lower_buffer[i]; ++i) {
			lower_buffer[i] = (char)tolower((unsigned char)lower_buffer[i]);
		}
		// Check for exit commands. If the user types "/quit" or "/exit" in any case, close the socket and exit
		if (strcmp(lower_buffer, "/quit") == 0 || strcmp(lower_buffer, "/exit") == 0)
		{
			closesocket(sock);
			WSACleanup();
			break;
		}

		// Prepend nickname to the message
		std::string messageToSend = userNickname + ": " + buffer;
		send(sock, messageToSend.c_str(), (int)messageToSend.length(), 0);
    }  

    closesocket(sock);  
    WSACleanup();  
}