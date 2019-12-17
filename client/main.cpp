#include <iostream>
#include <WS2TCPIP.h>
#include <string>
#include <chrono>
#include <time.h>
#include <future>

#pragma comment (lib, "ws2_32.lib")
#define BUFLEN 4096
#define PORT 7777
using namespace std;

class clinetSocket
{
public:
	bool create(PCSTR ip) {
		//create hint struct for server
		server.sin_family = AF_INET;
		server.sin_port = htons(7777);

		if (inet_pton(AF_INET, ip, &server.sin_addr) == -1) {
			cout << "cannot do the inet_pton: " << WSAGetLastError() << endl;
			return false;
		}

		//create socket
		out = socket(AF_INET, SOCK_DGRAM, 0);
		if (out == INVALID_SOCKET) {
			cout << "cannot init socket: " << endl;
			return false;
		}
		DWORD nonBlocking = 1;
		if (ioctlsocket(out,
			FIONBIO,
			&nonBlocking) != 0)
		{
			printf("failed to set non-blocking\n");
			return false;
		}
		return true;
	}
	void close() {
		closesocket(out);
	}
	void send(string in) {
		int sendStatus = sendto(out, in.c_str(), in.size() + 1, 0, (sockaddr*)&server, sizeof(server));
		if (sendStatus == SOCKET_ERROR)
		{
			cerr << "Send failed, error code: " << WSAGetLastError() << endl;
		}
	}
	void recieve(char buf[]) 
	{
		ZeroMemory(buf, BUFLEN);
		rec = recvfrom(out, buf, BUFLEN, 0,(sockaddr*)&server, &slen);
		if (rec == SOCKET_ERROR) {
			//cerr << "Error recieving stuff. Error code: " << WSAGetLastError() << endl;
			return;
		}
		char clientIp[256];
		ZeroMemory(clientIp, 256);

		inet_ntop(AF_INET, &server.sin_addr, clientIp, 256);

		cout << "recieved: " << buf << " from " << clientIp << endl;
	}

private:
	SOCKET out;
	sockaddr_in server;
	int slen = sizeof(server);
	int rec;
};

string GetLineFromCin() {
	std::string line;
	std::getline(std::cin, line);
	return line;
}

void main()
{
	cout << "starting" << endl;
	string sendHeader = "62";

	//Init winsock
	WSADATA wsaData;
	WORD ver = MAKEWORD(2, 2); //Use version 2.2

	int status = WSAStartup(ver, &wsaData);//Start winsock
	if (status != 0) {
		cerr << "Winsock couldn't start!!" << endl; //Throw error if cant start
		cin.get();
		return;
	}
	//Create socket
	clinetSocket mySocket;
	cout << "Enter ip to connect: ";
	string ipAddr;
	std::getline(std::cin, ipAddr);
	if (!mySocket.create(ipAddr.c_str())) {
		cin.get();
		exit(EXIT_FAILURE);
	}

	//Write to socket
	char buf[BUFLEN];
	auto s = std::async(std::launch::async, GetLineFromCin);
	double seconds_since_start;
	time_t startTime = time(0);
	while (true) 
	{
		//send periodic message to let server know client is still alive and open
		seconds_since_start = difftime(time(0), startTime);
		if (seconds_since_start > 2)
		{
			startTime = time(0);
			mySocket.send(sendHeader);
		}


		if (s.wait_for(std::chrono::milliseconds(10)) == std::future_status::ready) 
		{
			auto in = s.get();
			if (in == "") {
				break;
			}

			mySocket.send(sendHeader+in);
			s = std::async(std::launch::async, GetLineFromCin);
		}
		mySocket.recieve(buf);
		Sleep(10);
	}
	

	//Close the socket
	mySocket.close();
	//Close winsock
	WSACleanup();

}
