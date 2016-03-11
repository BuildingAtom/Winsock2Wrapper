#include "Winsock2Wrapper.h"
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <sstream>

#pragma region Constructors
/**
*	The default constructor for the Winsock2Wrapper object.
*	It will construct a basic TCP addrinfo structure with the defaults above.
*/
Winsock2Wrapper::Winsock2Wrapper() : Winsock2Wrapper(false) {}

/**
*	A constructor with the option of creating a UDP addrinfo structure
*	instead of a TCP one.
*
*	If a true value is passed, a UDP addrinfo structure is created.
*/
Winsock2Wrapper::Winsock2Wrapper(bool udpEnabled) {
	if (udpEnabled)
		Winsock2Wrapper::createAddrInfo(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	else
		Winsock2Wrapper::createAddrInfo(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);
	Winsock2Wrapper::udpEnabled = udpEnabled;
}

/**
*	A constructor that takes custom inputs for the components of the
*	addrinfo structure.
*	(AI_FAMILY, AI_SOCKTYPE, AI_PROTOCOL)
*/
Winsock2Wrapper::Winsock2Wrapper(int family, int socktype, int protocol) {
	//Create addrinfo structure
	Winsock2Wrapper::createAddrInfo(family, socktype, protocol);
}

/**
*	A constructor that takes an existing socket.
*/
Winsock2Wrapper::Winsock2Wrapper(SOCKET socket) : connectSocket(socket) {
	//The Socket is copied in the initialization list
	
	//these are active variables
	connected = true;
	client = true;
	closed = false;
	doNotClose = true;

	//initialize the stringstream for readuntil / clear it
	inputUntil.clear();
	inputUntil.str("");
}

/**
*	Utility Function for Constructors
*/
void Winsock2Wrapper::createAddrInfo(int family, int socktype, int protocol) {
	//Create addrinfo structure
	SecureZeroMemory(&hints, sizeof(hints));	//Fill this block of memory with zeroes before continuing
	hints.ai_family = family;
	hints.ai_socktype = socktype;
	hints.ai_protocol = protocol;

	//initialize the stringstream for readuntil / clear it
	inputUntil.clear();
	inputUntil.str("");
}
#pragma endregion The various constructors for this object

#pragma region Operator Overloads
/**
*	Copy Constructor
*/
Winsock2Wrapper::Winsock2Wrapper(const Winsock2Wrapper& other) {
	//initialize Winsock with the set winsock version
	iResult = WSAStartup(WINSOCK_VER, &wsaData);
	if (iResult != 0) {
		if (DEBUG_ENABLED) printf("WSAStartup failed: %d\n", iResult);
	}
	if (DEBUG_ENABLED) printf("Winsock Initialized\n");

	result = other.result;
	ptr = other.ptr;

	//Duplicate Hints
	SecureZeroMemory(&hints, sizeof(hints));
	hints = other.hints;

	//Duplicate Socket
	connectSocket = other.connectSocket;

	//Duplicate the sockaddr thingy
	senderAddr = other.senderAddr;
	senderAddrSize = other.senderAddrSize;

	//Duplicate Flags
	client = other.client;
	connected = other.connected;
	closed = other.closed;
	customFlag = other.customFlag;
	udpEnabled = other.udpEnabled;
	recvCalledUdp = other.recvCalledUdp;


	//initialize the stringstream for readuntil / clear it
	inputUntil.clear();
	inputUntil.str("");
	//Then copy ot
	inputUntil << other.inputUntil.str();
}

/**
*	Move Constructor
*/
Winsock2Wrapper::Winsock2Wrapper(Winsock2Wrapper&& other) noexcept{
	//initialize Winsock with the set winsock version
	iResult = WSAStartup(WINSOCK_VER, &wsaData);
	if (iResult != 0) {
		if (DEBUG_ENABLED) printf("WSAStartup failed: %d\n", iResult);
	}
	if (DEBUG_ENABLED) printf("Winsock Initialized\n");

	//Duplicate Hints
	SecureZeroMemory(&hints, sizeof(hints));
	hints = other.hints;

	//Duplicate Socket
	connectSocket = other.connectSocket;
	other.connectSocket = INVALID_SOCKET;

	//Duplicate the sockaddr thingy
	senderAddr = other.senderAddr;
	senderAddrSize = other.senderAddrSize;

	//Duplicate Flags
	client = other.client;
	connected = other.connected;
	closed = other.closed;
	customFlag = other.customFlag;
	udpEnabled = other.udpEnabled;
	recvCalledUdp = other.recvCalledUdp;


	//initialize the stringstream for readuntil / clear it
	inputUntil.clear();
	inputUntil.str("");
	//Then copy ot
	inputUntil << other.inputUntil.str();
}

/**
*	Copy Assignment Operator
*/
Winsock2Wrapper& Winsock2Wrapper::operator=(const Winsock2Wrapper& other) {
	Winsock2Wrapper tmp(other);         // re-use copy-constructor
	*this = std::move(tmp); // re-use move-assignment
	return *this;
}

/**
*	Move Assignment Operator
*/
Winsock2Wrapper& Winsock2Wrapper::operator=(Winsock2Wrapper&& other) noexcept{
	//initialize Winsock with the set winsock version
	iResult = WSAStartup(WINSOCK_VER, &wsaData);
	if (iResult != 0) {
		if (DEBUG_ENABLED) printf("WSAStartup failed: %d\n", iResult);
	}
	if (DEBUG_ENABLED) printf("Winsock Initialized\n");

	//Duplicate Hints
	SecureZeroMemory(&hints, sizeof(hints));
	hints = other.hints;

	//Duplicate Socket
	connectSocket = other.connectSocket;
	other.connectSocket = INVALID_SOCKET;

	//Duplicate the sockaddr thingy
	senderAddr = other.senderAddr;
	senderAddrSize = other.senderAddrSize;

	//Duplicate Flags
	client = other.client;
	connected = other.connected;
	closed = other.closed;
	customFlag = other.customFlag;
	udpEnabled = other.udpEnabled;
	recvCalledUdp = other.recvCalledUdp;


	//initialize the stringstream for readuntil / clear it
	inputUntil.clear();
	inputUntil.str("");
	//Then copy ot
	inputUntil << other.inputUntil.str();

	return *this;
}
#pragma endregion The Rule of Five and completely broken...

#pragma region Destructor
/**
*	Destructor for the Winsock2Wrapper Object
*/
Winsock2Wrapper::~Winsock2Wrapper() noexcept {
	//runs the close function if the destructor is called before the close function.
	if (!closed)
		close();
}
#pragma endregion

#pragma region Client-Specific Functions
/**
*	Default connection
*/
Winsock2Wrapper::Result Winsock2Wrapper::connect() {
	return Winsock2Wrapper::connect(DEFAULT_HOSTNAME, DEFAULT_PORT);
}

/**
*	Custom hostname, Default port
*/
Winsock2Wrapper::Result Winsock2Wrapper::connect(std::string hostname) {
	return Winsock2Wrapper::connect(hostname, DEFAULT_PORT);
}

/**
*	Custom hostname and port
*/
Winsock2Wrapper::Result Winsock2Wrapper::connect(std::string hostname, int port) {
	if (!closed)
		return Winsock2Wrapper::Result::ALREADY_CONNECTED;

	//initialize Winsock with the set winsock version
	iResult = WSAStartup(WINSOCK_VER, &wsaData);
	if (iResult != 0) {
		if (DEBUG_ENABLED) printf("WSAStartup failed: %d\n", iResult);
		return (Winsock2Wrapper::Result)iResult;
	}
	if (DEBUG_ENABLED) printf("Winsock Initialized\n");

	//Resolve the serveraddress & port
	iResult = getaddrinfo(hostname.c_str(), std::to_string(port).c_str(), &hints, &result);
	if (iResult != 0) {
		if (DEBUG_ENABLED) printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return (Winsock2Wrapper::Result)iResult;
	}

	//Attempt to connect to the address returned by getaddrinfo
	ptr = result;

	//and create a socket for connect attempt to the server
	connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	if (connectSocket == INVALID_SOCKET) {
		if (DEBUG_ENABLED) printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		//todo Add later
		return Winsock2Wrapper::Result::UNKNOWN_SOCKET_ERROR;
	}
	if (DEBUG_ENABLED) printf("Socket Created\n");

	if (udpEnabled) {
		memset((char *)&senderAddr, 0, sizeof(senderAddr));
		senderAddr.sin_family = AF_INET;
		senderAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		senderAddr.sin_port = htons(0);
		// Setup the udp port for receive
		iResult = ::bind(connectSocket, (struct sockaddr *)&senderAddr, sizeof(senderAddr));
		if (iResult == SOCKET_ERROR) {
			if (DEBUG_ENABLED) printf("bind failed with error: %d\n", WSAGetLastError());
			Winsock2Wrapper::Result lastErrorResult = (Winsock2Wrapper::Result)WSAGetLastError();
			freeaddrinfo(result);
			closesocket(connectSocket);
			WSACleanup();
			return lastErrorResult;
		}
		if (DEBUG_ENABLED) printf("Socket Bound\n");
	}

	//Connect to the server and perform check
	iResult = ::connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(connectSocket);
		connectSocket = INVALID_SOCKET;
	}

	//Should try the next address returned by getaddrinfo, but naw, that isn't even an option because we didn't ask for more than one.
	freeaddrinfo(result);

	if (connectSocket == INVALID_SOCKET) {
		if (DEBUG_ENABLED) printf("Unable to connect to server!\n");
		WSACleanup();
		return Winsock2Wrapper::Result::UNKNOWN_SOCKET_ERROR;
	}
	if (DEBUG_ENABLED) printf("Connected\n");

	//SUCCESS
	connected = true;
	client = true;
	closed = false;
	return Winsock2Wrapper::Result::SUCCESS;
}
#pragma endregion

#pragma region Server-Specific Functions
/**
*	Create a server and bind to the default port. If tcp, listen also starts (wip)
*/
Winsock2Wrapper::Result Winsock2Wrapper::createServer() {
	return createServer(DEFAULT_PORT);
}

/**
*	Create a server and bind to the given port. If tcp, listen also starts (wip)
*/
Winsock2Wrapper::Result Winsock2Wrapper::createServer(int port) {
	if (!closed)
		return Winsock2Wrapper::Result::ALREADY_CONNECTED;

	//initialize Winsock with the set winsock version
	iResult = WSAStartup(WINSOCK_VER, &wsaData);
	if (iResult != 0) {
		if (DEBUG_ENABLED) printf("WSAStartup failed: %d\n", iResult);
		return (Winsock2Wrapper::Result)iResult;
	}
	if (DEBUG_ENABLED) printf("Winsock Initialized\n");

	//Set the flag if not custom set
	if (!customFlag)
		hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &result);
	if (iResult != 0) {
		if (DEBUG_ENABLED) printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return (Winsock2Wrapper::Result)iResult;
	}

	// Create a SOCKET for connecting to server
	connectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (connectSocket == INVALID_SOCKET) {
		if (DEBUG_ENABLED) printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		//todo Add later
		return Winsock2Wrapper::Result::UNKNOWN_SOCKET_ERROR;
	}
	if (DEBUG_ENABLED) printf("Socket Created\n");

	// Setup the listening socket
	iResult = bind(connectSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		if (DEBUG_ENABLED) printf("bind failed with error: %d\n", WSAGetLastError());
		Winsock2Wrapper::Result lastErrorResult = (Winsock2Wrapper::Result)WSAGetLastError();
		freeaddrinfo(result);
		closesocket(connectSocket);
		WSACleanup();
		return lastErrorResult;
	}
	if (DEBUG_ENABLED) printf("Socket Bound\n");

	freeaddrinfo(result);

	// Unchecked
	if (!udpEnabled) {
		iResult = listen(connectSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR) {
			if (DEBUG_ENABLED) printf("listen failed with error: %d\n", WSAGetLastError());
			Winsock2Wrapper::Result lastErrorResult = (Winsock2Wrapper::Result)WSAGetLastError();
			closesocket(connectSocket);
			WSACleanup();
			return lastErrorResult;
		}
		if (DEBUG_ENABLED) printf("Listener Created\n");
	}
	//end unchecked

	//SUCCESS
	connected = true;
	client = false;
	closed = false;
	return Winsock2Wrapper::Result::SUCCESS;
}

/**
*	Receive from UDP
*/
int Winsock2Wrapper::recvFrom(char * recvbuf, int recvbuflen) {
	if (client || !udpEnabled || !connected || closed)
		return 0;

	//Receive from socket
	iResult = recvfrom(connectSocket, recvbuf, recvbuflen,
						0, (sockaddr *)& senderAddr, &senderAddrSize);

	if (iResult >= 0) {
		if (DEBUG_ENABLED) printf("Bytes received: %d\n", iResult);
		recvCalledUdp = true;
		return iResult;
	}
	else {
		if (DEBUG_ENABLED) printf("recv failed: %d\n", WSAGetLastError());
		return -1;
	}
}

int Winsock2Wrapper::returnLastSender(sockaddr_in *sender) {
	*sender = senderAddr;
	return senderAddrSize;
}

/**
*	Send message to last UDP receiver
*/
Winsock2Wrapper::Result Winsock2Wrapper::sendTo(std::string message) {
	return sendTo(message, (sockaddr *)& senderAddr, senderAddrSize);

}

/**
*	Send message to specified UDP receiver
*/
Winsock2Wrapper::Result Winsock2Wrapper::sendTo(std::string message, sockaddr* receiver, int receiverSize) {
	if (!connected)
		return Winsock2Wrapper::Result::NOT_CONNECTED;
	if (client)
		return Winsock2Wrapper::Result::SOCKET_IS_CLIENT;

	iResult = sendto(connectSocket, message.c_str(), message.length(), 0, receiver, receiverSize);
	if (iResult == SOCKET_ERROR) {
		if (DEBUG_ENABLED) printf("sendto failed: %d\n", WSAGetLastError());
		close();
		return (Winsock2Wrapper::Result) WSAGetLastError();
	}

	if (DEBUG_ENABLED) printf("Bytes Sent: %ld\n", iResult);
	return Winsock2Wrapper::Result::SUCCESS;

}

/**
*	Accept a connection
*/
Winsock2Wrapper::Result Winsock2Wrapper::accept(Winsock2Wrapper *clientSocket) {
	SOCKET *tempSocket = new SOCKET;
	*tempSocket = ::accept(connectSocket, NULL, NULL);
	if (*tempSocket == INVALID_SOCKET) {
		if (DEBUG_ENABLED) printf("Accept failed: %d\n", WSAGetLastError());
		close();
		return (Winsock2Wrapper::Result) WSAGetLastError();
	}
	//Winsock2Wrapper *temp = new Winsock2Wrapper((SOCKET)*tempSocket);
	clientSocket = new Winsock2Wrapper((SOCKET)*tempSocket);

	tempSocket = nullptr;

	//clientSocket = temp;
	//temp = nullptr;

	delete tempSocket;
	
	while (clientSocket->receiveReady() != Winsock2Wrapper::CONNECTION_CLOSED) {
		//This isn't really neccesary, but why not?
		while (clientSocket->receiveReady() == Winsock2Wrapper::NO_DATA_RECEIVED) {}
		if (clientSocket->receiveReady() != Winsock2Wrapper::DATA_RECEIVED) {
			break;
		}

		std::string test = clientSocket->readUntil("\n");

		printf("%s\n", test.c_str());

		if (clientSocket->sendLine(test) == Result::CONNECTION_RESET)
			break;
	}

	return Winsock2Wrapper::SUCCESS;

	//clientSocket = new Winsock2Wrapper(tempSocket);
}
#pragma endregion

#pragma region Other Functions
/**
*	Sets the AI_FLAGS property of the addrinfo structure.
*	Should be called before connection of creating a listener socket.
*
*	Returns whether the function succeeded or not
*/
bool Winsock2Wrapper::setFlag(int flag) {
	if (connected || connectSocket != INVALID_SOCKET)
		return false;
	hints.ai_flags = flag;
	customFlag = true;
	return true;
}

//Clean up, Clean up, Everybody clean up!
/**
*	Closes the connection
*/
void Winsock2Wrapper::close() {
	if (closed || doNotClose)
		return;
	// shutdown the connection since we're done
	iResult = shutdown(connectSocket, SD_BOTH);
	if (iResult == SOCKET_ERROR) {
		if (DEBUG_ENABLED) printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return;
	}
	//Prevent miscounts. checks to make sure close isn't called before connection
	if (!connected)
		return;
	//Closes the socket
	closesocket(connectSocket);
	//Unloads the ws_32 dll or decreases the counter if it's loaded more than once
	WSACleanup();
	closed = true;
}
#pragma endregion

#pragma region Non-TCP-Listener-Socket Functions
/**
*	Sends a packet with a message of the string
*/
Winsock2Wrapper::Result Winsock2Wrapper::send(std::string message) {
	if (!connected)
		return Winsock2Wrapper::Result::NOT_CONNECTED;
	if (!client && !udpEnabled)
		return Winsock2Wrapper::Result::SOCKET_IS_LISTENER;

	if (!client && udpEnabled)
		return sendTo(message);

	//Send a packet of data
	iResult = ::send(connectSocket, message.c_str(), message.length(), 0);
	if (iResult == SOCKET_ERROR) {
		if (DEBUG_ENABLED) printf("send failed: %d\n", WSAGetLastError());
		close();
		return (Winsock2Wrapper::Result) WSAGetLastError();
	}

	if (DEBUG_ENABLED) printf("Bytes Sent: %ld\n", iResult);
	return Winsock2Wrapper::Result::SUCCESS;
}

/**
*	Sends a packet with a message of the string followed by a "newline"
*/
Winsock2Wrapper::Result Winsock2Wrapper::sendLine(std::string message) {
	return send(message + NEW_LINE);
}

/**
*	Reads from the socket until the given deliminator is found is matched.
*	The deliminator is not in the result.
*
*	Function is blocking. Poll with receiveReady() to see if there is a message to read.
*/
std::string Winsock2Wrapper::readUntil(std::string delim) {
	//returns an empty string if there isn't a connection
	if (!connected || (!client && !udpEnabled))
		return "";

	//variables
	std::string line = "";
	int strlength;

	//loop to read data and append it to the stringstream until it finds the next of the deliminating character
	do {
		//Receive the data
		strlength = rawreceive(recvbuf, buflen - 1);

		//Make sure the connection's not closed
		if (strlength > 0) {
			//Adds a terminating charactor to the end of the returned string
			//Then streams it into the stringstream
			recvbuf[strlength] = '\0';
			inputUntil << recvbuf;
		}
		//Receive returns a 0 if the connection doesn't exist or it has been closed.
		else if (strlength == 0) {
			//Break out of the loop and...
			break;
		}
	} while (inputUntil.str().find(delim) == std::string::npos);	//Checking for the deliminator

																	//...return an empty string!
	if (strlength == 0) {
		return "";
	}
	//holds stringstream so it can be cleared and store the data following the deliminator
	line = inputUntil.str();
	inputUntil.clear();
	inputUntil.str(line.substr(line.find(delim) + delim.length()));
	return line.substr(0, line.find(delim));
}

/**
*	Reads from the socket until a "newline" is reached.
*
*	Should use the sendLine function for this since this function is not
*	checking for a standard newline.
*
*	Function is blocking. Poll with receiveReady() to see if there is a message to read.
*/
std::string Winsock2Wrapper::readLine() {
	return readUntil(NEW_LINE);
}

/**
*	Checks to see if the socket is ready to receive.
*/
Winsock2Wrapper::Result Winsock2Wrapper::receiveReady() {
	if (!connected)
		return Winsock2Wrapper::Result::NOT_CONNECTED;
	if (!client)
		return Winsock2Wrapper::Result::SOCKET_IS_LISTENER;

	//Check if there's already data in the buffer
	if (!inputUntil.str().empty())
		return Winsock2Wrapper::Result::DATA_RECEIVED;

	char c;				//Dummy variable
	u_long iMode = 1;	//For the blocking mode of the socket, 1 is non-blocking

	iResult = ioctlsocket(connectSocket, FIONBIO, &iMode);	//Set the socket to non-blocking.
	if (iResult != NO_ERROR) {
		if (DEBUG_ENABLED) printf("ioctlsocket failed with error: %ld\n", iResult);
		return (Winsock2Wrapper::Result) ::WSAGetLastError();
	}

	//Read the next character in the buffer without deleting it.
	int i = recv(connectSocket, &c, 1, MSG_PEEK);
	//* Receive returns a 0 if the connection's not there and a value less than one if there's nothing
	//   If there's something, it returns the length of the data received

	iMode = 0;			//0 is blocking
	iResult = ioctlsocket(connectSocket, FIONBIO, &iMode);	//Set te socket mode back to blocking.
	if (iResult != NO_ERROR) {
		if (DEBUG_ENABLED) printf("ioctlsocket failed with error: %ld\n", iResult);
		return (Winsock2Wrapper::Result) ::WSAGetLastError();
	}

	//Check the length of data received.
	if (i > 0) {
		return Winsock2Wrapper::Result::DATA_RECEIVED;	//Return 1 (Packet in Buffer) if data is there
	}
	else if (i == 0 && !udpEnabled) {
		return Winsock2Wrapper::Result::CONNECTION_CLOSED;	//Return -1 (connection doesn't exist) if it receives nothing
	}
	else {
		return Winsock2Wrapper::Result::NO_DATA_RECEIVED;	//Return 0 (Nothing Here!) if there isn't any data
	}
}
/*
It converts the socket to be non-blocking to check the data in the buffer without
deleteing it or forcing the program to wait, then changes the socket back to
blocking and returning 1 if it finds data. 0 if it doesn't.
*/
#pragma endregion

//Receive until end of packet or until buffer full as defined by the recvbuflen variable
//Because of it's pointer to the recvbuf, it actually modifies the passed variable directly. The returned values depends on the success of fail of the ability for it to receive data.
int Winsock2Wrapper::rawreceive(char * recvbuf, int recvbuflen) {
	if (udpEnabled && !client) {
		return recvFrom(recvbuf, recvbuflen);
	}

	//returns a 0 if there isn't a connection or if it's not a client
	if (!connected || !client)
		return 0;

	//iResult just becomes the # of bytes received / strlength
	iResult = recv(connectSocket, recvbuf, recvbuflen, 0);

	if (iResult > 0) {
		if (DEBUG_ENABLED) printf("Bytes received: %d\n", iResult);
		return iResult;
	}
	else if (iResult == 0) {
		if (DEBUG_ENABLED && !udpEnabled) printf("Connection Closed\n");
		return 0;
	}
	else {
		if (DEBUG_ENABLED) printf("recv failed: %d\n", WSAGetLastError());
		return -1;
	}
}