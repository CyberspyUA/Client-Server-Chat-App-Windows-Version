
#include <iostream>  


/**  
 * @file main.cpp  
 * @brief Entry point for the chat application. Allows user to choose server or client mode.  
 *  
 * Prompts the user to select whether to run as a server or a client, then calls  
 * the appropriate initialization function.  
 *  
 * @author Nikita Struk  
 * @date May 30, 2025  
 */  

void InitializeServer();  
void InitializeClient();  

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
		InitializeClient();  
	}  
	else  
	{  
		std::cout << "Invalid choice. Please restart the application and select 1 or 2." << std::endl;  
		return 1;  
	}  
	return 0; 
}