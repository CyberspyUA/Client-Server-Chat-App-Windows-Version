# Windows Multi-Client Chat Application

## Overview

This project is a simple multi-client chat application for Windows, implemented in C++14 using the Winsock2 API. The application allows users to run either as a server or a client from a single executable, providing a basic command-line interface for selection.

- **Server:** Listens for incoming TCP connections on port 8080, accepts up to 10 clients, and relays messages between them. Handles client disconnections and uses `select()` for multiplexing.
- **Client:** Connects to the server, sends user-typed messages, and receives messages from the server asynchronously using a separate thread.

## Features

- Multi-client support (up to 10 clients)
- Real-time message broadcasting between clients
- Simple CLI for mode selection (server/client)
- Asynchronous message reception on the client side
- Clean resource management and error handling

## Usage

1. **Build the project** using Visual Studio 2022 or any compatible C++14 compiler on Windows.
2. **Run the executable.**  
   You will be prompted to choose:
   - `1` to run as Server
   - `2` to run as Client

3. **Server Mode:**  
   The server will start listening on port 8080 and display connection/disconnection events and relayed messages.

4. **Client Mode:**  
   The client will connect to the server at `127.0.0.1:8080`. You can type messages to send to all other connected clients.

## Requirements

- Windows OS
- Visual Studio 2022 (or compatible C++14 compiler)
- Winsock2 library (linked automatically via pragma)

## Authors

- Nikita Struk

## License

This project is provided for educational purposes.
