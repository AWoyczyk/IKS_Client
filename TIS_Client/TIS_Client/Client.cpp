#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <iostream>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <vector>
#include "tinyxml2.h"

#pragma once


union Value101 {
	struct {
		float value;
		unsigned char flag;
	} data;
	unsigned char bytes [5];
};

struct InfoDM {
	int numB;
	int numBTypes;
};

struct InfoCD {
	int numPB;
	int numFB;
	int numTB;
};

struct InfoPB {
	char* manName;
	int manId;
	char* deviceId;
	char* hwRev;
	char* swRev;
};

struct InfoTB {
	char* unitName;
	int unitId;
};

struct InfoFB {
	double value;
};

std::vector<std::string> blockTypes = { "Physical Block", "Transducer Block", "Function Block" };
std::vector<std::string> pbTypes = {"Transmitter", "Actuator", "Discrete I/O", "Controller", "Analyser", "Lab Device"};
std::vector<std::string> tbTypes = {"Pressure", "Temperature", "Flow", "Level", "Actuator", "Discrete I/O", "Analyzer", "Auxiliary", "Alarm"};
std::vector<std::string> fbTypes = {"Input", "Output", "Control", "Advanced Control", "Calculation", "Auxiliary", "Alert"};

tinyxml2::XMLDocument manIdTable;

using namespace std;
int readparam(unsigned char flag, unsigned char address, unsigned char slot, unsigned char index) {
	//unsigned int iSockFd = INVALID_SOCKET;

	SOCKET iSockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(iSockFd == INVALID_SOCKET) {
		printf("socket fail\n");
		printf("%d\n", WSAGetLastError());
		exit(1);
	}

	struct sockaddr_in myAddr;
	memset(&myAddr, 0, sizeof(myAddr));

	myAddr.sin_family = AF_INET;
	myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myAddr.sin_port = htons(12344);

	int bindRet = bind(iSockFd, (struct sockaddr*)&myAddr, sizeof(myAddr));

	if (bindRet == -1) {
		printf("bind fail\n");
		exit(1);
	}

	unsigned char sbuf[4];
	sbuf[0] = flag;
	sbuf[1] = address;
	sbuf[2] = slot;
	sbuf[3] = index;

	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr("141.76.82.170");
	serverAddr.sin_port = htons(12345);

	int serverLen = sizeof(serverAddr);

	printf("sending...\n");

	int sendRes = sendto(iSockFd, reinterpret_cast<const char*>(sbuf), 4, 0, (struct sockaddr*)&serverAddr, serverLen);

	if (sendRes != 4) {
		printf("send fail %d\n", sendRes);
		exit(1);
	}
	printf("ok.\n");

	struct sockaddr_in cliAddr;
	memset(&cliAddr, 0, sizeof(cliAddr));
	int cliLen = sizeof(cliAddr);

	unsigned char buff[1024];

	printf("receiving...\n");
	int iRcvdBytes = recvfrom(iSockFd, reinterpret_cast<char*>(buff), 1024, 0, (struct sockaddr*)&cliAddr, &cliLen);

	if (iRcvdBytes > 0) {
		printf("received %3d bytes\n", iRcvdBytes-1);
		if (buff[0] == sbuf[0]) {
			int i;
			for (i = 1; i < iRcvdBytes; i++) {
				printf(" %3u,", buff[i]);
			}
			printf("\n == \n");
			for (i = 1; i < iRcvdBytes; i++) {
				printf(" 0x%02X,", buff[i]);
			}
			printf("\n == \n");
			for (i = 1; i < iRcvdBytes; i++) {
				printf(" %c,", buff[i]);
			}
			printf("\n");
		}
		else {
			printf("%d\n", WSAGetLastError());
			printf("reveive fail: frame marker differs - sent: %d != rvcd: %d\n", sbuf[0], buff[0]);
		}
	}
	else {
		printf("%d\n", WSAGetLastError());
		printf("receive fail %d\n", iRcvdBytes);
	}

	closesocket(iSockFd);
}

int getName(int id, const char* name) {
	const char* name_str;
	tinyxml2::XMLElement* level = manIdTable.FirstChildElement()->FirstChildElement("Manufacturer");
	for (level; level; level = level->NextSiblingElement("Manufacturer")) {
		const char* id_str;
		id_str = level->Attribute("ID");
		int id_int = stoi(id_str);
		if (id_int == id) {
			//printf("FOUND: ");
			tinyxml2::XMLElement* nameEle = level->FirstChildElement("ManufacturerInfo")->FirstChildElement("ManufacturerName");
			name_str = nameEle->GetText();
			//printf("%d = %s", id_int, name_str);
			name = name_str;
			return 1;
		}
	}
	return 0;
}

int main(int argc, char **argv) {
	manIdTable.LoadFile("Man_ID_Table.xml");

	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* Winsock DLL.                                  */
		printf("WSAStartup failed with error: %d\n", err);
		return 1;
	}

	/* Test if ID is in xml
	const char* name;
	int test = getName(123, name);
	*/


	unsigned char fla = 0xff & rand();
	unsigned int add = 6;
	unsigned int slo = 1;
	unsigned int idx = 0;
	while (true) {
		printf("\nPlease enter Address of Device:");
		cin >> add;
		/*printf("\nSlot:");
		cin >> slo;
		printf("\nIndex:");
		cin >> idx;*/
		readparam(fla, add, slo, idx);
	}
	
}