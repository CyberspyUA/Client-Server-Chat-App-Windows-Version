/**
 * @file main.cpp
 * @brief Entry point for the chat application. Allows user to choose server or client mode.
 *
 * Prompts the user to select whether to run as a server or a client, then calls
 * the appropriate initialization function.
 *
 *
 * @author Nikita Struk
 * @date May 30, 2025
 * Last updated: June 3, 2025
 */
#include <iostream>  
#include <string>



void InitializeServer();  
void InitializeClient(const std::string& serverAddress, 
					  const unsigned int& serverPort, 
					  mutable std::string userNickname);

int main()  
{  
	int choice = 0;  
	// Display welcome message and options  
	std::cout << "Welcome to the Chat Application!" << std::endl;  
	std::cout << "1. Run as Server\n";  
	std::cout << "2. Run as Client\n";  
	std::cout << "Enter your choice (1 or 2): ";  
	std::cin >> choice;  

	// Clear input buffer in case of leftover newline  
	std::cin.ignore(10000, '\n');  

	if (choice == 1)  
	{  
		InitializeServer();  
	}  
	else if (choice == 2)  
	{  
		std::cout << "Input the server IP address: \n";
		std::string serverAddress{};
		std::cin >> serverAddress;
		std::cout << "Input the server IP port: \n";
		unsigned int serverPort = 0;
		std::cin >> serverPort;
		//Prompt for nickname
		std::string userNickname;
		std::cout << "Enter your nickname: ";
        // Clear input buffer before reading nickname
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::getline(std::cin, userNickname);
		if (userNickname.empty())
		{
			userNickname = "Anonymous"; // Default nickname if none provided
			std::cout << "No nickname provided. Using default: " << userNickname << std::endl;
		}

		std::cout << "Starting in client mode..." << std::endl;
		InitializeClient(serverAddress, serverPort, userNickname);
	}  
	else  
	{  
		std::cout << "Invalid choice. Please restart the application and select 1 or 2." << std::endl;  
		return 1;  
	}  
	return 0; 
}