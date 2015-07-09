#include "Winsock2Wrapper.h"
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <sstream>

//the constructor
Winsock2Wrapper::Winsock2Wrapper(){
	//initialize Winsock as 2.2
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	//error check
	if (iResult != 0){
		printf("WSAStartup failed: %d\n", iResult);
	}

	//Create addrinfo structure
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	//initialize stringstream / clear it
	inputLine.clear();
	inputLine.str("");
}

//connector
int Winsock2Wrapper::conn(std::string hostname, int port){
	//Resolve the serveraddress & port
	iResult = getaddrinfo(hostname.c_str(), std::to_string(port).c_str(), &hints, &result);
	//error check
	if (iResult != 0){
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 0;
	}

	//Attempt to connect to the address returned by getaddrinfo
	ptr = result;

	//create a socket for connecting to the server
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	if (ConnectSocket == INVALID_SOCKET){
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 0;
	}

	//Connect to the server and perform check
	iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR){
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}

	//Should try the next address returned by getaddrinfo, but naw, that isn't even an option
	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET){
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 0;
	}

	//SUCCESS
	return 1;
}

//Redirects to the above using the default values
int Winsock2Wrapper::conn(){
	return conn(DEFAULT_HOSTNAME, (int)DEFAULT_PORT);
}


//Couldn't use the name send becuase send is used by winsock...
int Winsock2Wrapper::output(std::string message){
	//Send a packet of data
	iResult = send(ConnectSocket, message.c_str(), message.length(), 0);
	if (iResult == SOCKET_ERROR){
		printf("send failed: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 0;
	}
	
	//Was for debug
	//printf("Bytes Sent: %ld\n", iResult);
	return 1;
}

//Receive until end of packet or until buffer full as defined by the recvbuflen variable
//Because of it's pointer to the recvbuf, it actually modifies the passed variable directly. The returned values depends on the success of fail of the ability for it to receive data.
int Winsock2Wrapper::rawreceive(char * recvbuf, int recvbuflen){
	iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
	if (iResult > 0){
		//printf("Bytes received: %d\n", iResult);
		return iResult;
	}
	else if (iResult == 0){
		printf("Connection Closed\n");
		return 0;
	}
	else{
		printf("recv failed: %d\n", WSAGetLastError());
		return -1;
	}
}

//It's basically calling rawreceive until it reads a newline character. So it's a way to work with streams or badly written servers in other languages.
//Unfortunately it also runs the risk of deleting data in the event that there is anything after the newline character... That should be fixed, but once again, I was in a rush
std::string Winsock2Wrapper::readLine(){
	char recvbuf[DEFAULT_BUFLEN];
	std::string line;
	int strlength;
	do{
		strlength = rawreceive(recvbuf, (int)DEFAULT_BUFLEN);
		if (strlength > 0){
			recvbuf[strlength] = '\0';
			inputLine << recvbuf;
		}
		if (strlength == 0){
			break;
		}
	} while (inputLine.str().find('\n') == -1);
	if (strlength == 0){
		return "<logout>The connection was closed by the remote host.</logout>";
	}
	std::getline(inputLine, line);
	inputLine.clear();
	inputLine.str("");
	return line;
}

//TODO: Return -1 if the connection was closed
//IT returns 1 if there is anything found in the buffer 0 for anything else
int Winsock2Wrapper::receiveReady(){
	char c;
	u_long iMode = 1;
	iResult = ioctlsocket(ConnectSocket, FIONBIO, &iMode);
	if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);
	int i = recv(ConnectSocket, &c, 1, MSG_PEEK);
	iMode = 0;
	iResult = ioctlsocket(ConnectSocket, FIONBIO, &iMode);
	if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);
	if (i > 0) {
		return 1;
	}
	else if (i == 0) {
		return 0;
	}
	else {
		return 0;
	}
}

//Clean up, Clean up, Everybody clean up!
void Winsock2Wrapper::close(){
	//Cleanup
	closesocket(ConnectSocket);
	WSACleanup();
}