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
		//unsigned char flag;
		float value;
	} data;
	unsigned char bytes [4];
};

union Unsigned16 {
	uint16_t uint;
	unsigned char bytes[2];
};

struct indexOffset {
	unsigned char offset;
	unsigned char index;
};

struct slotIndex {
	unsigned char index;
	unsigned char slot;
};

struct InfoDM {
	unsigned int numB;
	unsigned int numBTypes;
};

struct InfoCD {
	unsigned int numPB;
	unsigned int numFB;
	unsigned int numTB;
};

struct InfoPB {
	std::string manName;
	unsigned int manId;
	char deviceId[16];
	char hwRev[16];
	char swRev[16];
	std::string parentClass;
	std::string blockType;
};

struct InfoTB {
	std::string unitName;
	unsigned int childClass;
	std::string parentClass;
	std::string blockType;
	unsigned int unitId;
};

struct InfoFB {
	unsigned int childClass;
	std::string parentClass;
	std::string blockType;
	double value;
};

std::vector<std::string> blockTypes = {"-", "Physical Block", "Function Block", "Transducer Block"};
std::vector<std::string> pbTypes = {"-", "Transmitter", "Actuator", "Discrete I/O", "Controller", "Analyser", "Lab Device"};
std::vector<std::string> tbTypes = {"-", "Pressure", "Temperature", "Flow", "Level", "Actuator", "Discrete I/O", "Analyzer", "Auxiliary", "Alarm"};
std::vector<std::string> fbTypes = {"-", "Input", "Output", "Control", "Advanced Control", "Calculation", "Auxiliary", "Alert"};

tinyxml2::XMLDocument manIdTable;

using namespace std;
int readparam(unsigned char flag, unsigned char address, unsigned char slot, unsigned char index, unsigned char* response) {
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
	unsigned char* p = buff;

	printf("receiving...\n");
	int iRcvdBytes = recvfrom(iSockFd, reinterpret_cast<char*>(buff), 1024, 0, (struct sockaddr*)&cliAddr, &cliLen);

	if (iRcvdBytes > 1) {
		printf("received %3d bytes\n", iRcvdBytes-1);
		if (buff[0] == sbuf[0]) {
			memcpy(response, p+1, 1023 * sizeof(char));

			/*
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
			*/
		}
		else {
			printf("%d\n", WSAGetLastError());
			printf("reveive fail: frame marker differs - sent: %d != rvcd: %d\n", sbuf[0], buff[0]);
		}
	}
	else {
		printf("%d\n", WSAGetLastError());
		printf("receive fail %d\n", iRcvdBytes);
		exit(1);
	}

	closesocket(iSockFd);
	return iRcvdBytes - 1;
}

int getName(int id, char* name) {
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
			strcpy(name, name_str);
			return 0;
		}
	}
	return 1;
}

int getVal(void * dest, unsigned char* src, int numBytes) {
	for (int i = 0; i < numBytes; i++) {
		unsigned char byte = *(src + i);
		*((reinterpret_cast<unsigned char*>(dest))+(numBytes-(i+1))) = byte;
	}
	return 0;
}

int startRequest(unsigned int address) {
	int numRcvdB;
	Unsigned16 numDirObj;
	Unsigned16 numDirEntry;
	Unsigned16 numCompListDirEntry;
	Unsigned16 firstCompDirListDirEntry;
	unsigned char response[1024];
	Unsigned16 value16;
	Value101 value101;
	uint8_t value8;
	unsigned char str[16];
	indexOffset indexOffsetPB, indexOffsetFB, indexOffsetTB;
	slotIndex slotIndexAddr;
	InfoDM deviceManagement{};
	InfoCD compositeDirectory{};
	InfoPB pbInfo{};
	std::vector<InfoTB> tbs;
	std::vector<InfoFB> fbs;


	uint16_t add = address;
	uint16_t slo = 1;
	uint16_t idx = 0;
	unsigned char fla = 0xff & rand();

	//Device Management
	numRcvdB = readparam(fla, add, slo, idx, response);

	getVal(&numDirObj.bytes, response + 4, sizeof(numDirObj));
	getVal(&numDirEntry.bytes, response + 6, sizeof(numDirEntry));
	getVal(&numCompListDirEntry.bytes, response + 10, sizeof(numCompListDirEntry));
	getVal(&firstCompDirListDirEntry.bytes, response + 8, sizeof(firstCompDirListDirEntry));

	deviceManagement.numBTypes = numCompListDirEntry.uint;
	deviceManagement.numB = numDirEntry.uint - numCompListDirEntry.uint;

	//Directory List
	idx = firstCompDirListDirEntry.uint;
	numRcvdB = readparam(fla, add, slo, idx, response);
	getVal(&value16, response + 2, sizeof(value16));
	getVal(&indexOffsetPB, response, sizeof(value16));
	compositeDirectory.numPB = value16.uint;
	getVal(&value16, response + 6, sizeof(value16));
	getVal(&indexOffsetTB, response + 4, sizeof(value16));
	compositeDirectory.numTB = value16.uint;
	getVal(&value16, response + 10, sizeof(value16));
	getVal(&indexOffsetFB, response + 8, sizeof(value16));
	compositeDirectory.numFB = value16.uint;

	//Directory
	readparam(fla, add, slo, firstCompDirListDirEntry.uint, response);

	/*
	 * Physical Block:
	 */
	if (indexOffsetPB.index != idx) {
		numRcvdB = readparam(fla, add, slo, indexOffsetPB.index, response);
		indexOffsetPB.offset -= numCompListDirEntry.uint;
		indexOffsetTB.offset -= numCompListDirEntry.uint;
		indexOffsetFB.offset -= numCompListDirEntry.uint;
	}
	getVal(&slotIndexAddr, response + ((indexOffsetPB.offset - 1) * 4), sizeof(slotIndexAddr));
	if (address == 7) { slotIndexAddr.index = 114; } //Physical Block at another index as stated
	numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index, response);
	memcpy(&value8, response+1, 1);
	pbInfo.blockType = blockTypes.at(value8);
	memcpy(&value8, response + 2, 1);
	pbInfo.parentClass = pbTypes.at(value8);

	//Manufacturer
	numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 10, response);
	getVal(&value16, response, sizeof(value16));
	pbInfo.manId = value16.uint;
	char s[64];
	getName(pbInfo.manId, s);
	pbInfo.manName = s;
	//DeviceId
	numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 11, response);
	strcpy(pbInfo.deviceId, reinterpret_cast<char*>(response));
	//HW & SW Revision
	numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 9, response);
	strcpy(pbInfo.hwRev, reinterpret_cast<char*>(response));
	numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 8, response);
	strcpy(pbInfo.swRev, reinterpret_cast<char*>(response));

	//Transducer Blocks:
	for (int i = 0; i < compositeDirectory.numTB; i++) {
		if (address == 7) { break; } //Transducer Block broken?
		InfoTB info{};
		numRcvdB = readparam(fla, add, slo, indexOffsetTB.index, response);
		getVal(&slotIndexAddr, response + ((indexOffsetTB.offset - 1 + i) * 4), sizeof(slotIndexAddr));
		numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index, response);
		memcpy(&value8, response + 1, 1);
		info.blockType = blockTypes.at(value8);
		memcpy(&value8, response + 2, 1);
		info.parentClass = tbTypes.at(value8);
		memcpy(&value8, response + 3, 1);
		info.childClass = value8;
		if (info.parentClass == "Pressure" && info.childClass == 1) {

		}
		else if (info.parentClass == "Temperature" && info.childClass == 1) {

		}

		tbs.push_back(info);
	}

	//Function Blocks:
	for (int i = 0; i < compositeDirectory.numFB; i++) {
		if (address == 7 && i == compositeDirectory.numFB - 1) { break; } //Function Block broken?
		InfoFB info{};
		numRcvdB = readparam(fla, add, slo, indexOffsetFB.index, response);
		getVal(&slotIndexAddr, response + ((indexOffsetFB.offset - 1 + i) * 4), sizeof(slotIndexAddr));
		numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index, response);
		memcpy(&value8, response + 1, 1);
		info.blockType = blockTypes.at(value8);
		memcpy(&value8, response + 2, 1);
		info.parentClass = fbTypes.at(value8);
		memcpy(&value8, response + 3, 1);
		info.childClass = value8;
		if (info.parentClass == "Input" && info.childClass == 1) {
			numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index+10, response);
			getVal(&value101.bytes, response, 4);
			info.value = value101.data.value;
		}

		fbs.push_back(info);
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


	
	uint16_t add = 6;

	while (true) {
		printf("\nPlease enter Address of Device:");
		cin >> add;
		/*printf("\nSlot:");
		cin >> slo;
		printf("\nIndex:");
		cin >> idx;*/
		int resp = startRequest(add);
	}
	
}