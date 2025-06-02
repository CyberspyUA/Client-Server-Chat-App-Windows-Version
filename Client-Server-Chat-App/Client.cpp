#pragma once

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <atomic>
#include <thread>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

// Define the buffer size for sending and receiving messages
#define BUFFER_SIZE 1024

// Delay before attempting to reconnect in seconds
#define RECONNECT_DELAY_SECONDS 5 

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
 * - Attempts to reconnect if the connection is lost.
 * @author Nikita Struk
 * @date May 30, 2025
 * Last updated: June 2, 2025
 */

 // Shared flag to signal disconnection
std::atomic<bool> isDisconnected(false);

/**
 * @brief Function to receive messages from the server in a separate thread.
 * 
 * This function runs in a loop, receiving messages from the server and printing them to the console.
 * If the connection is lost, it sets the isDisconnected flag to true.
 * 
 * @param socket_desc Pointer to the socket descriptor.
 * @return DWORD Returns 0 on success.
*/

DWORD WINAPI receive_messages(LPVOID socket_desc) 
{
	int sock = *(int*)socket_desc;
	char buffer[BUFFER_SIZE];
	int valread;
	while (1) 
    {
		memset(buffer, 0, BUFFER_SIZE);
		valread = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (valread <= 0)
        {
			printf("Connection lost. Attempting to reconnect...\n");
			isDisconnected = true;
            break;
        }

		buffer[valread] = '\0';
		printf("Received: %s\n", buffer);
	}
	return 0;
}

/*
* Helper function to connect to the server
* @param sock Reference to the socket descriptor
* @param serverAddress The address of the server to connect to
* Returns true if connection is successful, false otherwise
*/

bool ConnectToServer(int& sock, const std::string& serverAddress, unsigned int serverPort)
{
	struct sockaddr_in serv_addr;
	sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) 
    {
        perror("Socket creation failed");
        return false;
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverAddress.c_str(), &serv_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        closesocket(sock);
        return false;
    }
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        closesocket(sock);
		return false;
    }
    return true;
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

reconnect_label:
	// If the user didn't connect successfully, try to reconnect up to "connectionAttempts" times
	unsigned int connectionAttempts = 0;
    isDisconnected = false;
    printf("Connecting to the server...\n");
    while (!ConnectToServer(sock, serverAddress, serverPort) && connectionAttempts < 3)
    {
        printf("Connection failed. Retrying in %d seconds...\n", RECONNECT_DELAY_SECONDS);
		std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_DELAY_SECONDS));
		connectionAttempts++;
        if (connectionAttempts >= 3)
        {
            printf("Failed to connect after %d attempts. Exiting...\n", connectionAttempts);
            WSACleanup();
            exit(EXIT_FAILURE);
		}
    }

    printf("Connected to the server.\n");

    HANDLE thread_handle = CreateThread(NULL, 0, receive_messages, &sock, 0, NULL);
    if (thread_handle == NULL)
    {
        perror("Thread creation failed");
        closesocket(sock);
        WSACleanup();
		exit(EXIT_FAILURE);
    }
    while (1)
    {
        printf("Enter message: ");
		fgets(buffer, BUFFER_SIZE, stdin);
        //Remove trailing newline from gets
		buffer[strcspn(buffer, "\n")] = 0;
		//Convert buffer to lowercase for case-insensitive comparison
		char lower_buffer[BUFFER_SIZE];
		strncpy_s(lower_buffer, buffer, BUFFER_SIZE);
		lower_buffer[BUFFER_SIZE - 1] = '\0'; // Ensure null termination
        for (int i = 0; lower_buffer[i]; i++)
            lower_buffer[i] = (char)tolower((unsigned char) lower_buffer[i]);
        /*
        * Check for exit commands.
		* If the user types "/exit" or "/quit" in any case, close the socket and exit.
        */
        if (strcmp(lower_buffer, "/quit") == 0 || strcmp(lower_buffer, "/exit") == 0)
        {
            closesocket(sock);
            WSACleanup();
            isDisconnected = false;
            return;
        }
        if (strlen(buffer) == 0)
        {
            printf("Message cannot be empty. Please enter a message.\n");
            continue;
        }
        //Check for overly long messages (after nickname is prepended)
		std::string messageWithNickname = userNickname + ": " + buffer;
        if (messageWithNickname.length() >= BUFFER_SIZE)
        {
            printf("Message too long. Please limit your message to %zu characters.\n",
				BUFFER_SIZE - userNickname.length() - 2); // 2 for ": "
			continue;
        }
        //If disconnected, break to reconnect
        if (isDisconnected)
            break;
		//Prepend the nickname to the message
        int sendResult = send(sock,
                              messageWithNickname.c_str(),
                              (int)messageWithNickname.length(), 0);
        if (sendResult == SOCKET_ERROR)
        {
            printf("Send failed. Attempting to reconnect...\n");
            isDisconnected = true;
            break;
        }
    }
    closesocket(sock);
	//Wait a moment to ensure the receive thread exits cleanly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (isDisconnected)
        goto reconnect_label;
    WSACleanup();
}