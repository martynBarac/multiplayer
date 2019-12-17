#include <iostream>
#include <WS2TCPIP.h>
#include <time.h>
#include <vector>

#pragma comment (lib, "ws2_32.lib")
#define BUFLEN 4096
#define PORT 7777
#define CONNECTION_REQUEST 432

using namespace std;



void main()
{
	cout << "starting" << endl;

	//Init winsock
	WSADATA wsaData;
	WORD ver = MAKEWORD(2, 2); //Use version 2.2

	int status = WSAStartup(ver, &wsaData);//Start winsock
	if (status != 0) {
		cerr << "Winsock couldn't start!!" << endl; //Throw error if cant start
		cin.get();
		return;
	}


	//Create UDP socket
	SOCKET sockUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //Create a socket called "listening"
	if (sockUDP == INVALID_SOCKET) {
		cerr << "Socket cant init!! Error code: " << WSAGetLastError() << endl; //Throw error if cant init
		cin.get();
		return;
	}

	DWORD nonBlocking = 1;
	if (ioctlsocket(sockUDP,
		FIONBIO,
		&nonBlocking) != 0)
	{
		printf("failed to set non-blocking\n");
		return;
	}

	//Bind the socket to an ip address & port
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(PORT); //bind port to 7777
	hint.sin_addr.S_un.S_addr = ADDR_ANY;//bind ip to LocalHost Serevr
	int hintlen = sizeof(hint);


	int bindStatus = bind(sockUDP, (sockaddr*)&hint, hintlen); // Do the bind
	if (bindStatus == SOCKET_ERROR) {
		cerr << "cant bind port doe, error code: " << WSAGetLastError() << endl;
		cin.get();
		closesocket(sockUDP);
		return;
	}
	
	sockaddr_in client;
	int clientlen = sizeof(client);
	ZeroMemory(&client, clientlen);
	//sockaddr_in clients[12]; // connected clients
	vector<sockaddr_in> clients;
	vector<time_t> clientTimouts;
	int clientsConnected = 0; //How many clients are in the server
	int maxclients = 12;
	char buf[BUFLEN];
	char thing[BUFLEN] = "a";
	int recieve;



	time_t startTime = time(0);
	double seconds_since_start;
	while (true) 
	{
		//send periodic message
		seconds_since_start = difftime(time(0), startTime);
		if (seconds_since_start > 15)
		{
			for (int i = 0; i < clientsConnected; i++) {
				string in = "You are currently connected to the server!";
				int clen = sizeof(clients.at(i));
				int sendStatus = sendto(sockUDP, in.c_str(), in.size() + 1, 0, (sockaddr*)&clients.at(i), clen);
				if (sendStatus == SOCKET_ERROR)
				{
					cerr << "Send failed, error code: " << WSAGetLastError() << endl;
				}
			}
			startTime = time(0);
		}


		//check for disconnections
		for (int i = 0; i < clientsConnected; i++)
		{
			if (difftime(time(0), clientTimouts.at(i)) > 5) //Warn client timeout
			{
				cout << "client " << i << " timed out" << endl;
				clients.erase(clients.begin() + i);
				clientTimouts.erase(clientTimouts.begin() + i);
				clientsConnected--;
			}
			else if (difftime(time(0), clientTimouts.at(i)) > 3) //Warn client timeout
			{
				cout << "client " << i << " about to time out" << endl;
			}
		}

		ZeroMemory(buf,BUFLEN);
		recieve = recvfrom(sockUDP, buf, BUFLEN, 0, (sockaddr*)& client, &clientlen);
		if (recieve == SOCKET_ERROR) {
			//cerr << "Error recieving stuff. Error code: " << WSAGetLastError() << endl;
			continue;
		}
		char clientIp[256];
		ZeroMemory(clientIp, 256);

		inet_ntop(AF_INET, &client.sin_addr, clientIp, 256);

		string message = buf;
		string header = message.substr(0, 2);
		message.erase(0, 2);
		
		

		
		//listen for connections
		if (header.compare((string)"62") == 0) //check if header good
		{
			//is list full?
			if (clientsConnected < maxclients)
			{
				//Check if this is a new connection
				bool newConnection = true;
				for (int i = 0; i < clientsConnected; i++) {
					sockaddr* claddr = (sockaddr*)&clients[i];
					if (memcmp(claddr, (sockaddr*)&client, clientlen) == 0) {

						//Message came from a client who is already connected so lets print the messgae!
						if (message.compare("") != 0)
						{
							cout << "recieved: " << message.c_str() << " from " << clientIp << endl;
						}

						clientTimouts.at(i) = time(0);
						newConnection = false;
						break;
					}
				}
				if (newConnection)
				{
					//Add connection to a list of connected adresses
					clients.push_back(client);
					clientTimouts.push_back(time(0));
					clientsConnected++;
					//Announce that a new person has connected
					cout << "client " << clientIp << " connected!" << endl;
					cout << clientsConnected << " connected clients" << endl;
				}
			}
		}
		
		Sleep(10); //Computer cpu too fast slow down
	}
	closesocket(sockUDP);
	//Close winsock
	WSACleanup();

}