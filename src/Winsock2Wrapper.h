#ifndef WINSOCK2WRAPPER_H
#define WINSOCK2WRAPPER_H
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <sstream>

//Based on code found from the MSDN examples and doc.
//todo Look into Rule of Three and Fix.

#pragma comment (lib, "Ws2_32.lib")

class Winsock2Wrapper {
public:
	#pragma region Constants
		const int DEFAULT_PORT = 80;
		const std::string DEFAULT_HOSTNAME = "127.0.0.1";
		const static int DEFAULT_BUFLEN = 2048;
		const bool DEBUG_ENABLED = true;
		const WORD WINSOCK_VER = MAKEWORD(2, 2);
		const std::string NEW_LINE = "\r\r\n\n";
	#pragma endregion
private:

	WSADATA wsaData;//Windows Socket Item
	int iResult;	//Integer result

	//Create addrinfo structure
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	//For UDP server, temporary sockaddr and stuff for client
	//For the client, holds the client info
	sockaddr_in senderAddr;
	int senderAddrSize = sizeof(senderAddr);

	//Create the socket object
	SOCKET connectSocket = INVALID_SOCKET;

	//For the read until character function (readUntil)
	std::stringstream inputUntil;

	//The buffer length for the functions to use on rawreceive
	int buflen = DEFAULT_BUFLEN;

	//The buffer used for functions on rawreceive
	char recvbuf[DEFAULT_BUFLEN];

	//internal flags
	bool client = false;
	bool connected = false;
	bool closed = true;
	bool customFlag = false;
	bool udpEnabled = false;

	bool doNotClose = false;

	bool recvCalledUdp = false;

	/**
	*	Utility Function for Constructors
	*/
	void createAddrInfo(int, int, int);

public:
#pragma region Result Enumeration
	typedef enum {
		SOCKET_IS_LISTENER = -3,
		NOT_CONNECTED = -2,
		SUCCESS,
		#pragma region WSAStartup Errors
			SUBSYS_NOT_READY = 10091,
			WINSOCK_VER_UNSUPPORTED,
			BLOCK_IN_PROGRESS = 10036,
			WINSOCK_TASK_LIMIT = 10067,
		#pragma endregion
		#pragma region getaddrinfo Errors
			AGAIN = 11002,
			NOT_ENOUGH_MEMORY = 8,
			RESOLVE_FAILURE = 11003,
			BAD_FLAG = 10022,
			BAD_FAMILY = 10047,
			HOST_NOT_RESOLVED = 11001,
			SERVICE_NOT_FOUND = 10109,
			SOCK_NOT_SUPPORTED = 10044,
		#pragma endregion
		#pragma region send Errors
			SUBSYS_FAILED = 10050,
			INTERRUPTED = 10004,
			//BLOCK_IN_PROGRESS,
			NETWORK_RESET = 10052,
			CONNECTION_ABORTED,
			CONNECTION_RESET,
			OPERATION_NOT_SUPPORTED = 10045,
			WOULD_BLOCK_NONBLOCKING = 10035,
			MSG_TOO_BIG = 10040,
			HOST_UNREACHABLE = 10065,
			//BAD_FLAG,
			CONNECTION_TIMEDOUT = 10060,
		#pragma endregion
		#pragma region receiveReady Results
			DATA_RECEIVED = -4,
			NO_DATA_RECEIVED = -5,
		#pragma endregion
		#pragma region bind	Results
			ADRRESS_IN_USE = 10048,
			ADDRESS_NOT_AVAILIBLE,
		#pragma endregion
		UNKNOWN_SOCKET_ERROR = -1,
		CONNECTION_CLOSED = -6,
		ALREADY_CONNECTED = -7,
		SOCKET_IS_CLIENT = -8,
	} Result;
#pragma endregion

#pragma region Constructors
	/**
	*	The default constructor for the Winsock2Wrapper object.
	*	It will construct a basic TCP addrinfo structure with the defaults above.
	*/
	Winsock2Wrapper();

	/**
	*	A constructor with the option of creating a UDP addrinfo structure
	*	instead of a TCP one.
	*
	*	If a true value is passed, a UDP addrinfo structure is created.
	*/
	Winsock2Wrapper(bool);

	/**
	*	A constructor that takes an existing socket.
	*/
	Winsock2Wrapper(SOCKET);

	/**
	*	A constructor that takes custom inputs for the components of the
	*	addrinfo structure.
	*	(AI_FAMILY, AI_SOCKTYPE, AI_PROTOCOL)
	*/
	Winsock2Wrapper(int, int, int);
#pragma endregion The various constructors for this object.

#pragma region Operator Overloads
	/**
	*	Copy Constructor
	*/
	Winsock2Wrapper(const Winsock2Wrapper&);

	/**
	*	Move Constructor
	*/
	Winsock2Wrapper(Winsock2Wrapper&&) noexcept;

	/**
	*	Copy Assignment Operator
	*/
	Winsock2Wrapper& operator=(const Winsock2Wrapper&);

	/**
	*	Move Assignment Operator
	*/
	Winsock2Wrapper& operator=(Winsock2Wrapper&&) noexcept;
#pragma endregion The Rule of Five and completely broken...
	
#pragma region Destructor
	/**
	*	Destructor for the Winsock2Wrapper Object
	*/
	~Winsock2Wrapper() noexcept;
#pragma endregion

#pragma region Client-Specific Functions
	/**
	*	Default connection
	*/
	Result connect();

	/**
	*	Custom hostname, Default port
	*/
	Result connect(std::string);

	/**
	*	Custom hostname and port
	*/
	Result connect(std::string, int);
#pragma endregion

#pragma region Server-Specific Functions
	/**
	*	Create a server and bind to the default port. If tcp, listen also starts (wip)
	*/
	Result createServer();

	/**
	*	Create a server and bind to the given port. If tcp, listen also starts (wip)
	*/
	Result createServer(int);
	
	#pragma region UDP-Specific
		/**
		*	Receive from UDP
		*/
		private: int recvFrom(char*, int);

		/**
		*	Returns the last address recvFrom found.
		*/
		public: int returnLastSender(sockaddr_in *);

		/**
		*	Send message to last UDP receiver
		*/
		Result sendTo(std::string);

		/**
		*	Send message to specified UDP receiver
		*/
		Result sendTo(std::string, sockaddr*, int);

		/**
		*	Accept a connection. Argument is the Winsock2Wrapper Object of the returned connection.
		*/
		Result accept(Winsock2Wrapper*);
	#pragma endregion
#pragma endregion

#pragma region Non-TCP-Listener-Socket Functions
	/**
	*	Sends a packet with a message of the string
	*/
	Result send(std::string);

	/**
	*	Sends a packet with a message of the string followed by a "newline"
	*/
	Result sendLine(std::string);

	/**
	*	Reads from the socket until the given deliminator is matched.
	*	The deliminator is not in the result.
	*
	*	Function is blocking. Poll with receiveReady() to see if there is a message to read.
	*/
	std::string readUntil(std::string);

	/**
	*	Reads from the socket until a "newline" is reached.
	*
	*	Should use the sendLine function for this since this function is not
	*	checking for a standard newline.
	*
	*	Function is blocking. Poll with receiveReady() to see if there is a message to read.
	*/
	std::string readLine();

	/**
	*	Checks to see if the socket is ready to receive.
	*/
	Result receiveReady();
	/*
		It converts the socket to be non-blocking to check the data in the buffer without
		deleteing it or forcing the program to wait, then changes the socket back to
		blocking and returning 1 if it finds data. 0 if it doesn't.
	*/
#pragma endregion

#pragma region Other Functions
	/**
	*	Sets the AI_FLAGS property of the addrinfo structure.
	*	Should be called before connection of creating a listener socket.
	*	
	*	Returns whether the function succeeded or not
	*/
	bool setFlag(int);

	/**
	*	Close the connection
	*/
	void close();

	//Raw Receive (receive by buflen)
	//Socket is blocking! Will freeze program if called without anything in the buffer
	//Receive until recvbufflen, or until end of message
	int rawreceive(char*, int);
#pragma endregion

};
#endif