#include "Winsock2Wrapper.h"
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <sstream>

//The constructor for the wrapper
//Constructor for the Winsock2Wrapper object with custom buffer length
Winsock2Wrapper::Winsock2Wrapper(int a){
	//Create addrinfo structure
	SecureZeroMemory(&hints, sizeof(hints));	//Fill this block of memory with zeroes before continuing
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	//initialize the stringstream for readuntil / clear it
	inputUntil.clear();
	inputUntil.str("");

	//The buffer, will throw exception if failed
	recvbuf = new char[buflen];
}

//Destructor for the Winsock2Wrapper object
Winsock2Wrapper::~Winsock2Wrapper(){
	//runs the close function if the destructor is called before the close function.
	if (!closed)
		close();
	// Deallocate the memory that was previously reserved
	//  for the rawrecieve buffer by other functions
	if (recvbuf)
		delete[] recvbuf;
}

//connector
int Winsock2Wrapper::conn(std::string hostname, int port){
	//initialize Winsock as version 2.2
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	//error check
	if (iResult != 0){
		printf("WSAStartup failed: %d\n", iResult);
	}

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

	//Should try the next address returned by getaddrinfo, but naw, that isn't even an option because we didn't ask for more than one.
	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET){
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 0;
	}

	//SUCCESS
	connected = true;
	return 1;
}

//Redirects to the above using the default values
int Winsock2Wrapper::conn(){
	return conn(DEFAULT_HOSTNAME, (int)DEFAULT_PORT);
}


//Couldn't use the name send becuase send is used by winsock...
int Winsock2Wrapper::output(std::string message){
	//returns a -1 if there isn't a connection
	if (!connected)
		return -1;

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
	//returns a 0 if there isn't a connection
	if (!connected)
		return 0;

	//iResult just becomes the # of bytes received / strlength
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

//This function calls rawreceive in a loop until it encounters the charcter in the argument.
//Unfortunately it also runs the risk of deleting data in the event that there is anything
//after the deliminating character... That should be fixed, soon.
//EDIT: Has been fixed, but most likely is buggy because it hasn't been tested
//      The receiveReady function most likely will have to also check the stringstream as well
std::string Winsock2Wrapper::readUntil(char delim){
	//returns an empty string if there isn't a connection
	if (!connected)
		return "";

	//variables
	std::string line = "";
	int strlength;

	//loop to read data and append it to the stringstream until it finds the next of the deliminating character
	do{
		//Receive the data
		strlength = rawreceive(recvbuf, buflen-1);

		//Make sure the connection's not closed
		if (strlength > 0){
			//Adds a terminating charactor to the end of the returned string
			//Then streams it into the stringstream
			recvbuf[strlength] = '\0';
			inputUntil << recvbuf;
		}
		//Receive returns a 0 if the connection doesn't exist or it has been closed.
		else if (strlength == 0){
			//Break out of the loop and...
			break;
		}
	} while (inputUntil.str().find(delim) == -1);	//Checking for the deliminator
	//...return an empty string!
	if (strlength == 0){
		return "";
	}
	//holds stringstream so it can be cleared and store the data following the deliminator
	line = inputUntil.str();
	inputUntil.clear();
	inputUntil.str(line.substr(line.find(delim) + 1));
	return line.substr(0, line.find(delim));
}

//Returns 1 if anything is found, 0 if not, and -1 if connection doesn't exist
int Winsock2Wrapper::receiveReady(){
	//returns a -1 if there isn't a connection
	if (!connected)
		return -1;

	char c;				//Dummy variable
	u_long iMode = 1;	//For the blocking mode of the socket, 1 is non-blocking

	iResult = ioctlsocket(ConnectSocket, FIONBIO, &iMode);	//Set the socket to non-blocking.
	if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);
	
	//Read the next character in the buffer without deleting it.
	int i = recv(ConnectSocket, &c, 1, MSG_PEEK);
	//* Receive returns a 0 if the connection's not there and a value less than one if there's nothing
	//   If there's something, it returns the length of the data received

	iMode = 0;			//0 is blocking
	iResult = ioctlsocket(ConnectSocket, FIONBIO, &iMode);	//Set te socket mode back to blocking.
	if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);

	//Check the length of data received.
	if (i > 0) {
		return 1;	//Return 1 (Packet in Buffer) if data is there
	}
	else if (i == 0) {
		return -1;	//Return -1 (connection doesn't exist) if it receives nothing
	}
	else {
		return 0;	//Return 0 (Nothing Here!) if there isn't any data
	}
}

//Clean up, Clean up, Everybody clean up!
void Winsock2Wrapper::close(){
	//Prevent miscounts. checks to make sure close isn't called before connection
	if (!connected)
		return;
	//Closes the socket
	closesocket(ConnectSocket);\
	//Unloads the ws_32 dll or decreases the counter if it's loaded more than once
	WSACleanup();
	closed = true;
}
