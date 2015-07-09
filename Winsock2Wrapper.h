#ifndef WINSOCK2WRAPPER_H
#define WINSOCK2WRAPPER_H
#include <WinSock2.h>
#include <ws2tcpip.1h>
#include <stdio.h>
#include <string>
#include <sstream>

//Based on code found from the MSDN Winsock2 Client Example

#pragma comment (lib, "Ws2_32.lib")

//Defaults
#define DEFAULT_PORT "7636"
#define DEFAULT_HOSTNAME "127.0.0.1"
#define DEFAULT_BUFLEN 1024

class Winsock2Wrapper{
private:
	WSADATA wsaData;						//Windows Socket Item
	int iResult;							//Integer result

	//Create addrinfo structure
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	SOCKET ConnectSocket = INVALID_SOCKET;	//Create the socket

	//For the line by line read (the readLine function)
	std::stringstream inputLine;

public:
	//constructor
	Winsock2Wrapper();

	//Connect
	int conn(std::string hostname, int port);
	//Overload for the above function that would connect to the defaults above
	int conn();

	//Send a packet of the message
	int output(std::string message);

	//Raw Receive (receive by packet)
	//Socket is blocking! Will freeze program if called without anything in the buffer
	int rawreceive(char * recvbuf, int recvbuflen);

	//Return all received data up to a newline. (might delete data after, so poll receiveReady to call readline asap after data reaches the client)
	std::string readLine();

	//Check if the socket is ready to receive
	//This function was a last second addition. Unfortunately, It doesn't detect if the server suddenly shut down.
	//It converts the socket to be non-blocking check the data in the buffer without deleteing it, then changing the socket back to blocking and returning 1 if it finds data. 0 if it doesn't.
	int receiveReady();

	//Close the connection
	void close();
};
#endif