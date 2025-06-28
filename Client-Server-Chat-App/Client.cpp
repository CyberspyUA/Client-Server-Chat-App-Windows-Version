#pragma once
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
 * - Uses colored text for system, user, and error messages.
 * @author Nikita Struk
 * @date May 30, 2025
 * Last updated: June 19, 2025
 */

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
#include <fstream>
#include <iomanip> // Adding this include for std::put_time



#pragma comment(lib, "ws2_32.lib")

// Define the buffer size for sending and receiving messages
#define BUFFER_SIZE 1024

// Delay before attempting to reconnect in seconds
#define RECONNECT_DELAY_SECONDS 5 

// Console color codes for Windows
#define COLOR_DEFAULT 7
#define COLOR_SYSTEM 11
#define COLOR_USER 10
#define COLOR_ERROR 12



//User-customizable color variables
WORD g_colorDefault = COLOR_DEFAULT;
WORD g_colorSystem = COLOR_SYSTEM;
WORD g_colorUser = COLOR_USER;
WORD g_colorError = COLOR_ERROR;

void PrintSystem(const char* message);

// Helper function to print available color codes
void PrintColorHelp()
{
	PrintSystem("Available color codes (foreground):\n");
	printf("0: Black\n1: Blue\n2: Green\n3: Aqua\n4: Red\n5: Purple\n6: Yellow\n7: White\n8: Gray");
	printf("\n9: Light Blue\n10: Light Green\n11: Light Aqua\n12: Light Red\n13: Light Purple\n14: Light Yellow\n15: Bright White\n");
	PrintSystem("Usage: /color <type> <code>\n");
	PrintSystem("Types: system, user, error, default\n");
}

 // Shared flag to signal disconnection
std::atomic<bool> isDisconnected(false);

//Helper function to set console text color
void SetConsoleColor(WORD color)
{
    static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
}

//Print system/info message in cyan color
void PrintSystem(const char* message)
{
    SetConsoleColor(g_colorSystem);
    printf("%s", message);
    SetConsoleColor(g_colorDefault);
}

//Print user message in green color
void PrintUser(const char* message)
{
    SetConsoleColor(g_colorUser);
    printf("%s", message);
	SetConsoleColor(g_colorDefault);
}

// Print error message in red color
void PrintError(const char* message)
{
    SetConsoleColor(g_colorError);
    printf("%s", message);
    SetConsoleColor(g_colorDefault);
}

void HandleColorCommand(const char* buffer)
{
	// Format: /color <type> <code>
	char type[16];
	int code;
	// FIX: Pass buffer size for %s argument as required by sscanf_s
	int matched = sscanf_s(buffer, "/color %15s %d", type, (unsigned)_countof(type), &code);

	if (matched != 2 || code < 0 || code > 15)
	{
		PrintColorHelp();
		return;
	}

	if (strcmp(type, "system") == 0)
	{
		g_colorSystem = (WORD)code;
		PrintSystem("System message color updated.\n");
	}
	else if (strcmp(type, "user") == 0)
	{
		g_colorUser = (WORD)code;
		PrintSystem("User message color updated.\n");
	}
	else if (strcmp(type, "error") == 0)
	{
		g_colorError = (WORD)code;
		PrintSystem("Error message color updated.\n");
	}
	else if (strcmp(type, "default") == 0)
	{
		g_colorDefault = (WORD)code;
		PrintSystem("Default color updated.\n");
	}
	else
	{
		PrintColorHelp();
	}
}

// Modify the LogMessage function to fix the issue
void LogMessage(const std::string& message)
{
    std::ofstream logFile("client_log.txt", std::ios::app);
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
        PrintError("Failed to open log file.\n");
    }
}
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
			PrintSystem("Connection lost. Attempting to reconnect...\n");
			isDisconnected = true;
            break;
        }

		buffer[valread] = '\0';

		MessageBeep(MB_ICONEXCLAMATION); // Beep to notify user of new message)

        // Print user messages in green, system messages in cyan
		// Heuristic: if message contains ":", it's a user message, else system
        if (strstr(buffer, ": ") == buffer + 0)
        {
            //Unlikely, but fallback
			PrintUser(buffer);
            printf("\n");
        }
        else if (strstr(buffer, ": "))
        {
			PrintUser(buffer);
            printf("\n");
        }
        else
		{
            PrintSystem(buffer);
			printf("\n");
        }
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
        PrintError("Socket creation failed");
        return false;
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverAddress.c_str(), &serv_addr.sin_addr) <= 0)
    {
        PrintError("Invalid address");
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

void InitializeClient(const std::string& serverAddress, 
                      const unsigned int& serverPort, 
                      std::string userNickname)
{  
    WSADATA wsaData;
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        PrintError("WSAStartup failed\n");
        exit(EXIT_FAILURE);
    }

reconnect_label:
	// If the user didn't connect successfully, try to reconnect up to "connectionAttempts" times
	unsigned int connectionAttempts = 0;
    isDisconnected = false;
    PrintSystem("Connecting to the server...\n");
    while (!ConnectToServer(sock, serverAddress, serverPort) && connectionAttempts < 3)
    {
		PrintError("Connection failed. Retrying...\n");
        SetConsoleColor(g_colorSystem);
        printf("%d", RECONNECT_DELAY_SECONDS);
        SetConsoleColor(g_colorError);
		printf(" seconds...\n");
        SetConsoleColor(g_colorDefault);
        std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_DELAY_SECONDS));
		connectionAttempts++;
        if (connectionAttempts >= 3)
        {
            PrintError("Failed to connect after 3 attempts. Exiting...\n");
            WSACleanup();
			exit(EXIT_FAILURE);
        }
    }

    PrintSystem("Connected to the server.\n");

    HANDLE thread_handle = CreateThread(NULL, 0, receive_messages, &sock, 0, NULL);
    if (thread_handle == NULL)
    {
        PrintError("Thread creation failed");
        closesocket(sock);
        WSACleanup();
		exit(EXIT_FAILURE);
    }
    while (1)
    {
        PrintSystem("Enter message: ");
		fgets(buffer, BUFFER_SIZE, stdin);
        //Remove trailing newline from gets
		buffer[strcspn(buffer, "\n")] = 0;
		// Handle /color command locally
		if (strncmp(buffer, "/color", 6) == 0)
		{
			if (strcmp(buffer, "/color help") == 0 || strcmp(buffer, "/color") == 0) {
				PrintColorHelp();
			}
			else
			{
				HandleColorCommand(buffer);
			}
			// Clear buffer to avoid accidental send
			memset(buffer, 0, BUFFER_SIZE);
			continue;
		}
        //Handle /users command: send to server, display response
        if (strcmp(buffer, "/users") == 0)
        {
			int sendResult = send(sock, "/users", (int)strlen(buffer), 0);
            if (sendResult == SOCKET_ERROR)
            {
                PrintError("Failed to request user list. Attempting to reconnect...\n");
                isDisconnected = true;
                break;
            }
			//The server should respond with the user list, which will be displayed by receive_messages thread.
            continue;

        }
		// Handle /nick command
        if (strncmp(buffer, "/nick", 5) == 0)
        {
			std::string newNickname = buffer + 5; // Skip "/nick "
			// Trim whitespace from the new nickname
            newNickname.erase(0, newNickname.find_first_not_of(" \t\r\n"));
			newNickname.erase(newNickname.find_last_not_of(" \t\r\n") + 1);
            if(newNickname.empty())
            {
                PrintError("Nickname cannot be empty. Please enter a valid nickname.\n");
				continue;
            }
            if (newNickname.length() > 32)
            {
                PrintError("Nickname too long (max 32 characters). \n");
				continue;
            }
            std::string oldNickname = userNickname;
			userNickname = newNickname; // Update the nickname
			//Inform the server about the nickname change
            std::string sysMsg = oldNickname + " changed nickname to " + userNickname;
			int sendResult = send(sock, sysMsg.c_str(), (int)sysMsg.length(), 0);
            LogMessage("[NICK] " + sysMsg);
            if (sendResult == SOCKET_ERROR)
            {
                // Fix: Ensure `userNickname` is a mutable variable and matches the type of `newNickname`.  
                // Update the declaration of `userNickname` to be a `std::string` if it isn't already.  

                std::string userNickname; // Ensure this is declared as a std::string  

                // Modify the assignment to ensure compatibility.  
                userNickname = newNickname; // Update the nickname
				PrintError("Failed to send nickname change to server. Attempting to reconnect.\n");
                isDisconnected = true;
                break;
            }
			PrintSystem("Nickname updated\n");
            continue;
        }


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
            PrintError("Message cannot be empty. Please enter a message.\n");
            continue;
        }
        //Check for overly long messages (after nickname is prepended)
		std::string messageWithNickname = userNickname + ": " + buffer;
        if (messageWithNickname.length() >= BUFFER_SIZE)
        {
			PrintError("Message too long. Please limit your message to ");
			SetConsoleColor(COLOR_SYSTEM);
			printf("%zu", BUFFER_SIZE - userNickname.length() - 2); // 2 for ": "
			SetConsoleColor(COLOR_ERROR);
			printf(" characters.\n");
			SetConsoleColor(COLOR_DEFAULT);
        }
        //If disconnected, break to reconnect
        if (isDisconnected)
            break;
		//Prepend the nickname to the message
        int sendResult = send(sock,
                              messageWithNickname.c_str(),
                              (int)messageWithNickname.length(), 0);

		LogMessage(messageWithNickname); // Log the message with timestamp

        if (sendResult == SOCKET_ERROR)
        {
            PrintError("Send failed. Attempting to reconnect...\n");
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