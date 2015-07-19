/*
Simple Client Example using the Winsock2Wrapper.
Initiates a connection with google.com and waits for a response.
*/


#include "Winsock2Wrapper.h"
#include <cstdio>

using namespace std;

char buf[1024];	//Buffer for the recv function in the loop here
int l;			//Length of the returned data

int main(){
	//Create the Winsock2Wrapper object
	Winsock2Wrapper winsock2Wrapper;
	
	//Connect. If connection fails, end the program with return value -1
	if (!winsock2Wrapper.conn("google.com", 80))
		return -1;
	
	//Send a simple http request. Uses a conditional operator to return success or fail
	printf("%s\n\n", (winsock2Wrapper.output("GET / HTTP/1.1\r\nHost: google.com\r\n\r\n") ? "Success." : "Fail."));

	//Checks receiveReady for anything in the buffer, the if the connection failed or not
	while (winsock2Wrapper.receiveReady() != -1){
		//This isn't really neccesary, but why not?
		while (winsock2Wrapper.receiveReady() == 0){}
		
		//Read incoming data
		l = winsock2Wrapper.rawreceive(buf, 1024);

		//Output the returned data
		printf("%.*s\n", l, buf);
	}

	//Close the connection.
	winsock2Wrapper.close();

	return 0;
}
