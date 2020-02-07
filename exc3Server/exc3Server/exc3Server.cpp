#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <fstream>
#include<sstream>
using namespace std;

// Don't forget to include "Ws2_32.lib" in the library list.
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <cstdio>
#include <ctime>


struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	int sendSubType;	// Sending sub-type
	char buffer[1000];
	int len;
};

const int TIME_PORT = 27015;
const double TIMEOUT = 60.0;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int GET = 1;
const int POST = 2;
bool POSTVALID;

bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);
bool handlePost(int index);
string normelizeContent(string content);

struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;
std::clock_t socketTimer[MAX_SOCKETS];


void main()
{
	// Initialize Winsock (Windows Sockets).

	// Create a WSADATA object called wsaData.
	// The WSADATA structure contains information about the Windows 
	// Sockets implementation.
	WSAData wsaData;

	// Call WSAStartup and return its value as an integer and check for errors.
	// The WSAStartup function initiates the use of WS2_32.DLL by a process.
	// First parameter is the version number 2.2.
	// The WSACleanup function destructs the use of WS2_32.DLL by a process.
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Time Server: Error at WSAStartup()\n";
		return;
	}

	// Server side:
	// Create and bind a socket to an internet address.
	// Listen through the socket for incoming connections.

	// After initialization, a SOCKET object is ready to be instantiated.

	// Create a SOCKET object called listenSocket. 
	// For this application:	use the Internet address family (AF_INET), 
	//							streaming sockets (SOCK_STREAM), 
	//							and the TCP/IP protocol (IPPROTO_TCP).
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	// Error detection is a key part of successful networking code. 
	// If the socket call fails, it returns INVALID_SOCKET. 
	// The if statement in the previous code is used to catch any errors that
	// may have occurred while creating the socket. WSAGetLastError returns an 
	// error number associated with the last error that occurred.
	if (INVALID_SOCKET == listenSocket)
	{
		cout << "Time Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	// For a server to communicate on a network, it must bind the socket to 
	// a network address.

	// Need to assemble the required data for connection in sockaddr structure.

	// Create a sockaddr_in object called serverService. 
	sockaddr_in serverService;
	// Address family (must be AF_INET - Internet address family).
	serverService.sin_family = AF_INET;
	// IP address. The sin_addr is a union (s_addr is a unsigned long 
	// (4 bytes) data type).
	// inet_addr (Iternet address) is used to convert a string (char *) 
	// into unsigned long.
	// The IP address is INADDR_ANY to accept connections on all interfaces.
	serverService.sin_addr.s_addr = INADDR_ANY;
	// IP Port. The htons (host to network - short) function converts an
	// unsigned short from host to TCP/IP network byte order 
	// (which is big-endian).
	serverService.sin_port = htons(TIME_PORT);

	// Bind the socket for client's requests.

	// The bind function establishes a connection to a specified socket.
	// The function uses the socket handler, the sockaddr structure (which
	// defines properties of the desired connection) and the length of the
	// sockaddr structure (in bytes).
	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR*)& serverService, sizeof(serverService)))
	{
		cout << "Time Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	// Listen on the Socket for incoming connections.
	// This socket accepts only one connection (no pending connections 
	// from other clients). This sets the backlog parameter.
	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	addSocket(listenSocket, LISTEN);

	// Accept connections and handles them one by one.
	while (true)
	{
		// The select function determines the status of one or more sockets,
		// waiting if necessary, to perform asynchronous I/O. Use fd_sets for
		// sets of handles for reading, writing and exceptions. select gets "timeout" for waiting
		// and still performing other operations (Use NULL for blocking). Finally,
		// select returns the number of descriptors which are ready for use (use FD_ISSET
		// macro to check which descriptor in each set is ready to be used).
		fd_set waitRecv;
		FD_ZERO(&waitRecv);

		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		//
		// Wait for interesting event.
		// Note: First argument is ignored. The fourth is for exceptions.
		// And as written above the last is a timeout, hence we are blocked if nothing happens.
		//
		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, NULL);
		if (nfd == SOCKET_ERROR)
		{
			cout << "Time Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		double duration;
		for (int i = 1; i < MAX_SOCKETS; i++) {
			duration = (std::clock() - socketTimer[i]) / (double)CLOCKS_PER_SEC;
			if (sockets[i].recv != EMPTY && duration > TIMEOUT)
			{
				SOCKET id = sockets[i].id;
				cout << "Socket " << i << " Duration is " << duration << "\n";
				closesocket(id);
				removeSocket(i);
				cout << "Socket Timeout, Closing Socket \n" << i;
			}
		}
		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					socketTimer[i] = std::clock();
					acceptConnection(i);
					break;

				case RECEIVE:
					socketTimer[i] = std::clock();
					receiveMessage(i);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}
	}

	// Closing connections and Winsock.
	cout << "Time Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool addSocket(SOCKET id, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			socketTimer[i] = std::clock();
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index)
{
	socketTimer[index] = 0;
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr*) & from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "Time Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "Time Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	//
	// Set the socket to be in non-blocking mode.
	//
	unsigned long flag = 1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout << "Time Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}

	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "Time Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout << "Time Server: Recieved: " << bytesRecv << " bytes of \"" << &sockets[index].buffer[len] << "\" message.\n";

		sockets[index].len += bytesRecv;

		char* request;
		char* fileName;
		char copyBuffer[1000];
		strcpy(copyBuffer, sockets[index].buffer);
		char* copyBufferPtr = copyBuffer;
		const char search[2] = " ";
		request = strtok_s(copyBufferPtr, search, &copyBufferPtr);	// get first word
		fileName = strtok_s(copyBufferPtr, search, &copyBufferPtr); // get second word
		if (sockets[index].len > 0)
		{
			if (strncmp(request, "GET", 3) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = GET;
				memcpy(sockets[index].buffer, &sockets[index].buffer[10], sockets[index].len - bytesRecv);
				sockets[index].len -= 10;
				return;
			}
			else if (strncmp(request, "POST", 4) == 0)
			{
				sockets[index].send = SEND;
				sockets[index].sendSubType = POST;
				memcpy(sockets[index].buffer, &sockets[index].buffer[10], sockets[index].len - bytesRecv);
				sockets[index].len -= 10;
				return;
			}
			else if (strncmp(request, "EXIT", 4) == 0)
			{
				closesocket(msgSocket);
				removeSocket(index);
				return;
			}
		}
	}

}

void sendMessage(int index)
{
	int bytesSent = 0;
	char sendBuff[255];

	SOCKET msgSocket = sockets[index].id;

	char* request;
	char* fileName;
	char copyBuffer[1000];
	strcpy(copyBuffer, sockets[index].buffer);
	char* copyBufferPtr = copyBuffer;
	const char search[2] = " ";
	request = strtok_s(copyBufferPtr, search, &copyBufferPtr);	// get first word
	fileName = strtok_s(copyBufferPtr, search, &copyBufferPtr); // get second word
	/**
		getting the path of the file
	*/
	string name = fileName;
	stringstream nameSS;
	nameSS << "websites" << name;
	name = nameSS.str();
	ifstream file(name);
	int code;
	string content;

	if (sockets[index].sendSubType == GET)
	{
		// if we have found the file in the folder
		if (file.good())
		{
			code = 200;
			string currContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			content = currContent;
		}
		else
		{
			code = 404;
			content = "<h1>404 Not Found</h1>";
		}
		cout << content;
		file.close();
		/**
			creating the header of the http response
		*/
		stringstream headerSS;
		string header;
		headerSS << "HTTP/1.1 " << code << " OK\r\n" << "Content-Type: text/html\r\n" <<
			"Content-Length: " << content.size() << "\r\n\r\n" << content;
		header = headerSS.str();

		int size = header.size() + 1;
		int bytesSent = 0;
		bytesSent = send(msgSocket, header.c_str(), size, 0);
	}
	else if (sockets[index].sendSubType == POST)
	{
		POSTVALID = handlePost(index);
		// if the file that the client requested to post is valid and is not in the folder
		if (POSTVALID) {
			content = "<h2>The file was saved successfully!</h2>";
			code = 200;
			/**
				creating the header of the http response
			*/
			stringstream headerSS;
			string header;
			headerSS << "HTTP/1.1 " << code << " OK\r\n" << "Content-Type: text/html\r\n" <<
				"Content-Length: " << content.size() << "\r\n\r\n" << content;
			header = headerSS.str();

			int size = header.size() + 1;
			int bytesSent = 0;
			bytesSent = send(msgSocket, header.c_str(), size, 0);
		}
		// the file that the client requested to post is not valid because he is in the folder already
		else
		{
			content = "<h2>The file is already in the folder!</h2>";
			code = 404;

			stringstream headerSS;
			string header;
			headerSS << "HTTP/1.1 " << code << " OK\r\n" << "Content-Type: text/html\r\n" <<
				"Content-Length: " << content.size() << "\r\n\r\n" << content;
			header = headerSS.str();

			int size = header.size() + 1;
			int bytesSent = 0;
			bytesSent = send(msgSocket, header.c_str(), size, 0);
		}
	}

	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Time Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "Time Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";

	sockets[index].send = IDLE;
}

/**
	a method that organizing the content and file name and post it to the folder
*/
bool handlePost(int index) {
	string getBuffer(sockets[index].buffer);
	int fileNameIndex = getBuffer.find("?"); //finding the begining of the file name
	int fileNameEndIndex;
	if (fileNameIndex == -1) { //if we cant find the fileNameIndex by ? it means it is html post and not text post
		fileNameIndex = getBuffer.find("/");
		fileNameEndIndex = getBuffer.find("HTTP/1.1");
		fileNameEndIndex -= 1;
	}
	else
		fileNameEndIndex = getBuffer.find("="); //finding the end of the file name if it is text post
	// getting the file name from the substring
	string fileName = getBuffer.substr(fileNameIndex + 1, fileNameEndIndex - fileNameIndex - 1);

	int contentIndex;
	int endOfTextIndex;
	string content;

	if (fileName.find("html") != string::npos) {
		contentIndex = getBuffer.find("<html>"); //finding the begining of the content if it is html
		content = getBuffer.substr(contentIndex);
	}
	else {
		contentIndex = getBuffer.find("="); //finding the begining of the content if it is a text
		endOfTextIndex = getBuffer.find("HTTP/1.1"); //finding the end of the content if it is a text
		content = getBuffer.substr(contentIndex + 1, endOfTextIndex - contentIndex - 1);
	}
	// getting the content from the substring

	ifstream fileToCheck("websites//" + fileName);
	/**
	checks if the file is already in the folder
	*/
	if (fileToCheck.good()) {
		return false;
	}
	ofstream file("websites//" + fileName);
		/**
	if we get spaces in the buffer it apears in request as "%20" that we need to remove
	*/
	if(fileNameIndex != -1)
		content = normelizeContent(content);
	file << content;
	file.close();
	return true;
}


string normelizeContent(string content) {
	stringstream normelizeContent;
	int size = content.length() + 1;
	/**
	allocating memory to check the string as char array
	*/
	char* contentArr;
	contentArr = new (nothrow) char[size];
	if (contentArr == nullptr)
		cout << "Error: memory could not be allocated";
	string currWord;
	strcpy(contentArr, content.c_str());
	/**
	searching for "%20" in the array that indicates there is a space
	*/
	for (int i = 0; i < content.length(); i++) {
		if (i + 2 < content.length() && contentArr[i] == '%' && contentArr[i + 1] == '2' && contentArr[i + 2] == '0') {
			i += 2;
			normelizeContent << " ";
		}
		else
			normelizeContent << contentArr[i];
	}
	content = normelizeContent.str();
	delete[] contentArr; //releasing the allocation
	return content;
}
