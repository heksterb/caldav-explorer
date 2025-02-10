/*
	main
 
	Command-line entry point
 	for Management Proxy

	2019/04/05	Originated
 
	Copyright © 2019 by: Ben Hekster
*/

#include <iostream>

#include "NSocket.h"

using namespace Win32;


// initialize WinSock
static Socket::Library gWinSock;


/*	main
	Command-line entry point
*/
int main()
{
try {
	// resolve the local address and port to be used by the server
	struct addrinfoW hints;
	hints.ai_flags = AI_PASSIVE /* server socket */ | AI_NUMERICSERV /* numeric service number */;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_addrlen = 0;
	hints.ai_canonname = nullptr;
	hints.ai_addr = nullptr;
	hints.ai_next = nullptr;
	Socket::AddressInfo result(nullptr, L"27016", hints);

	// create a socket for the server to listen for client connections
	Socket::Socket listenSocket = socket(result.begin()->ai_family, result.begin()->ai_socktype, result.begin()->ai_protocol);
	
	// set up the listening socket
	if (bind(listenSocket, result.begin()->ai_addr, result.begin()->ai_addrlen)) throw WSAGetLastError();
	if (listen(listenSocket, SOMAXCONN)) throw WSAGetLastError();

	// accept a client socket
	Socket::Socket clientSocket = accept(listenSocket, NULL, NULL);

	// shut down the send half of the connection since no more data will be sent
	if (shutdown(clientSocket, SD_SEND)) throw WSAGetLastError();

	// receive until the peer shuts down the connection
	char recvbuf[512];
	while (const int result = recv(clientSocket, recvbuf, sizeof recvbuf, 0)) {
		if (result == SOCKET_ERROR) throw WSAGetLastError();
		
		std::cout << "bytes received: " << result;
		}
	}

catch (const char error[]) {
	std::cerr << "error: " << error << std::endl;
	}

catch (const unsigned long error) {
	std::cerr << "error " << error << std::endl;
	}

catch (...) {
	std::cerr << "error" << std::endl;
	}

return 0;
}
