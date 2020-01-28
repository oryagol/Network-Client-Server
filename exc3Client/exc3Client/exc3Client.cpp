#define  _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
using namespace std;
// Don't forget to include "Ws2_32.lib" in the library list.
#include <winsock2.h> 
#include <string.h>
#include <iomanip>
#include <string>

const int TIME_PORT = 27015;


void main()
{

	// Initialize Winsock (Windows Sockets).

	WSAData wsaData;
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Time Client: Error at WSAStartup()\n";
		return;
	}

	// Client side:
	// Create a socket and connect to an internet address.

	SOCKET connSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == connSocket)
	{
		cout << "Time Client: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	//
	// Binding the created socket to the IP address and port representing "us".
	//
	//Note: for clients, if the socket is left unbound, the first call for connect() will 
	//bind the socket with arbitrary properties (IP and Port)

	//If specific values are required, you can use the following code:

	//struct sockaddr_in me;
	//me.sin_family = AF_INET;
	//me.sin_addr.s_addr = INADDR_ANY;
	//me.sin_port = 0;
	//
	// Binding the created socket to the IP address and port representing "us".
	//
	//if (bind(connSocket, (struct sockaddr*)&me, sizeof(me)) == SOCKET_ERROR)
	//{
	//	cout<<"Time Server: Error at bind(): "<<WSAGetLastError()<<endl;
	//  closesocket(connSocket);
	//	WSACleanup();
	//  return;
	//}

	// For a client to communicate on a network, it must connect to a server.    
	// Need to assemble the required data for connection in sockaddr structure.
	// Create a sockaddr_in object called server. 
	sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(TIME_PORT);

	// Connect to server.

	// The connect function establishes a connection to a specified network 
	// address. The function uses the socket handler, the sockaddr structure 
	// (which defines properties of the desired connection) and the length of 
	// the sockaddr structure (in bytes).
	if (SOCKET_ERROR == connect(connSocket, (SOCKADDR*)& server, sizeof(server)))
	{
		cout << "Time Client: Error at connect(): " << WSAGetLastError() << endl;
		closesocket(connSocket);
		WSACleanup();
		return;
	}

	// Send and receive data.

	int bytesSent = 0;
	int bytesRecv = 0;
	char sendBuff[255];
	char recvBuff[255];
	string option;

	while (option != "Exit")
	{
		cout << "\nPlease choose an option:\n";
		cout << "GET [website name].\n";
		cout << "POST.\n";
		cout << "EXIT.\n";
		cout << ">> ";

		getline(cin, option);

		const string get = "GET";
		const string post = "POST";
		const string Exit = "EXIT";

		char* firstWord;
		char* secondWord;
		char copyBuffer[255];
		strcpy(copyBuffer, option.c_str());
		char* copyBufferPtr = copyBuffer;
		const char search[2] = " ";
		firstWord = strtok_s(copyBufferPtr, search, &copyBufferPtr);	// get first word
		secondWord = strtok_s(copyBufferPtr, search, &copyBufferPtr); // get second word

		if (firstWord == get || firstWord == post || firstWord == Exit)
			strcpy(sendBuff, option.c_str());

		// The send function sends data on a connected socket.
		// The buffer to be sent and its size are needed.
		// The last argument is an idicator specifying the way 
		// in which the call is made (0 for default).
		bytesSent = send(connSocket, sendBuff, (int)strlen(sendBuff), 0);
		if (SOCKET_ERROR == bytesSent)
		{
			cout << "Time Client: Error at send(): " << WSAGetLastError() << endl;
			closesocket(connSocket);
			WSACleanup();
			return;
		}
		cout << "Time Client: Sent: " << bytesSent << "/" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";

		// Gets the server's answer for options 1 and 2.
		if (option == get || option == post)
		{
			bytesRecv = recv(connSocket, recvBuff, 255, 0);
			if (SOCKET_ERROR == bytesRecv)
			{
				cout << "Time Client: Error at recv(): " << WSAGetLastError() << endl;
				closesocket(connSocket);
				WSACleanup();
				return;
			}
			if (bytesRecv == 0)
			{
				cout << "Server closed the connection\n";
				return;
			}

			recvBuff[bytesRecv] = '\0'; //add the null-terminating to make it a string
			cout << "Time Client: Recieved: " << bytesRecv << " bytes of \"" << recvBuff << "\" message.\n";
		}
		else if (option == Exit)
		{
			// Closing connections and Winsock.
			cout << "Time Client: Closing Connection.\n";
			closesocket(connSocket);
			WSACleanup();
			Sleep(1500);
			exit(0);
		}
	}
}