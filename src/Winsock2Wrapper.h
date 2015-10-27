#ifndef WINSOCK2WRAPPER_H
#define WINSOCK2WRAPPER_H
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <sstream>

//Based on code found from the MSDN Winsock2 Client Example

#pragma comment (lib, "Ws2_32.lib")

//Defaults
#define DEFAULT_PORT "80"
#define DEFAULT_HOSTNAME "127.0.0.1"
#define DEFAULT_BUFLEN 1024

class Winsock2Wrapper{
private:
	WSADATA wsaData;//Windows Socket Item
	int iResult;	//Integer result

	//Create addrinfo structure
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	//Create the socket object
	SOCKET ConnectSocket = INVALID_SOCKET;

	//For the read until character function (readUntil)
	std::stringstream inputUntil;

	//The buffer length for the functions to use on rawreceive
	int buflen = DEFAULT_BUFLEN;

	//The buffer used for functions on rawreceive
	char *recvbuf;

	//internal flags
	bool connected = false;
	bool closed = false;

public:
	//Default constructor for the Winsock2Wrapper object
	Winsock2Wrapper() : Winsock2Wrapper(DEFAULT_BUFLEN) {}
	//Constructor for the Winsock2Wrapper object with custom buffer length
	Winsock2Wrapper(int);

	//Destructor for the Winsock2Wrapper Object
	~Winsock2Wrapper();

	//Connect
	int conn(std::string, int);
	//Overload for the above function that would connect to the defaults above
	int conn();

	//Send a packet of the message
	int output(std::string);

	//Raw Receive (receive by packet)
	//Socket is blocking! Will freeze program if called without anything in the buffer
	//Receive until recvbufflen, or until end of message
	int rawreceive(char*, int);

	//Return all received data up to a deliminating character.
	std::string readUntil(char);

	//Check if the socket is ready to receive
	int receiveReady();
	//It converts the socket to be non-blocking to check the data in the buffer without
	//deleteing it or forcing the program to wait, then changes the socket back to
	//blocking and returning 1 if it finds data. 0 if it doesn't.

	//Close the connection
	void close();
};
#endif
