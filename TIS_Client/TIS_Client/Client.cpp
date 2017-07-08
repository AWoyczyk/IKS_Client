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
	char deviceId[17];
	char hwRev[17];
	char swRev[17];
	unsigned int pcID;
	unsigned int btID;
	std::string parentClass;
	std::string blockType;
};

struct InfoTB {
	unsigned int ccID;
	unsigned int pcID;
	unsigned int btID;
	unsigned int numValues;
	std::vector<unsigned int> unitId;
	std::vector<std::string> unitName;
	std::vector<float> value;
	std::vector<char> flag;
	std::string childClass;
	std::string parentClass;
	std::string blockType;
};

struct InfoFB {
	unsigned int ccID;
	unsigned int pcID;
	unsigned int btID;
	std::string childClass;
	std::string parentClass;
	std::string blockType;
	double value;
	char flag;
	std::string unit;
};

tinyxml2::XMLDocument manIdTable;
tinyxml2::XMLDocument blockTypes;
int failCounter = 0;

using namespace std;
int readparam(unsigned char flag, unsigned char address, unsigned char slot, unsigned char index, unsigned char* response) {
	//unsigned int iSockFd = INVALID_SOCKET;

	SOCKET iSockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(iSockFd == INVALID_SOCKET) {
		printf("\nsocket fail");
		printf("\n%d", WSAGetLastError());
		exit(1);
	}

	struct sockaddr_in myAddr;
	memset(&myAddr, 0, sizeof(myAddr));

	myAddr.sin_family = AF_INET;
	myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myAddr.sin_port = htons(12344);

	int bindRet = bind(iSockFd, (struct sockaddr*)&myAddr, sizeof(myAddr));

	if (bindRet == -1) {
		printf("\nbind fail");
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

	//printf("sending...\n");

	int sendRes = sendto(iSockFd, reinterpret_cast<const char*>(sbuf), 4, 0, (struct sockaddr*)&serverAddr, serverLen);

	if (sendRes != 4) {
		printf("\nsend fail %d", sendRes);
		exit(1);
	}
	//printf("ok.\n");

	struct sockaddr_in cliAddr;
	memset(&cliAddr, 0, sizeof(cliAddr));
	int cliLen = sizeof(cliAddr);

	unsigned char buff[1024];
	unsigned char* p = buff;

	//printf("receiving...\n");
	int iRcvdBytes = recvfrom(iSockFd, reinterpret_cast<char*>(buff), 1024, 0, (struct sockaddr*)&cliAddr, &cliLen);

	if (iRcvdBytes > 1) {
		//printf("received %3d bytes\n", iRcvdBytes-1);
		if (buff[0] == sbuf[0]) {
			memcpy(response, p+1, 1023 * sizeof(char));
			failCounter = 0;
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
			//printf("\n%d", WSAGetLastError());
			printf("\nreveive fail: frame marker differs - sent: %d != rvcd: %d", sbuf[0], buff[0]);
		}
	}
	else {
		//printf("\n%d", WSAGetLastError());
		printf("\nreceive fail: %d data bytes", iRcvdBytes-1);
		closesocket(iSockFd);
		failCounter++;
		if (failCounter >= 30) {
			printf("\nConnection Problems... Abort");
			return -1;
		}
		return 0;
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

int getBlock(unsigned int btID, unsigned int pcID, unsigned int ccID, char* bt, char* pc, char* cc) {
	const char* name_str;
	const char* id_str;
	unsigned int id;
	memset(bt, '\0', 64);
	memset(pc, '\0', 64);
	memset(cc, '\0', 64);
	tinyxml2::XMLElement* nameEle;
	tinyxml2::XMLElement* btLevel = blockTypes.FirstChildElement()->FirstChildElement("BlockType");
	for (btLevel; btLevel; btLevel = btLevel->NextSiblingElement("BlockType")) {
		id_str = btLevel->Attribute("ID");
		id = stoi(id_str);
		if (id == btID) {
			nameEle = btLevel->FirstChildElement("TypeName");
			name_str = nameEle->GetText();
			strcpy(bt, name_str);
			tinyxml2::XMLElement* pcLevel = btLevel->FirstChildElement("ParentClass");
			for (pcLevel; pcLevel; pcLevel = pcLevel->NextSiblingElement("ParentClass")) {
				id_str = pcLevel->Attribute("ID");
				id = stoi(id_str);
				if (id == pcID) {
					nameEle = pcLevel->FirstChildElement("PCName");
					name_str = nameEle->GetText();
					strcpy(pc, name_str);
					tinyxml2::XMLElement* ccLevel = pcLevel->FirstChildElement("Class");
					for (ccLevel; ccLevel; ccLevel = ccLevel->NextSiblingElement("Class")) {
						id_str = ccLevel->Attribute("ID");
						id = stoi(id_str);
						if (id == ccID) {
							nameEle = ccLevel->FirstChildElement("ClassName");
							name_str = nameEle->GetText();
							strcpy(cc, name_str);
						}
					}
				}
			}
		}
	}
	return 0;
}

int getVal(void * dest, unsigned char* src, int numBytes) {
	for (int i = 0; i < numBytes; i++) {
		unsigned char byte = *(src + i);
		*((reinterpret_cast<unsigned char*>(dest))+(numBytes-(i+1))) = byte;
	}
	return 0;
}

int getUnitName(unsigned int id, char* s) {
	switch (id) {
	case 1342: strcpy(s, "%\0"); break;
	case 1138: strcpy(s, "mbar\0"); break;
	default: strcpy(s, "\0"); break;
	}
	return 1;
}

int printElements(InfoDM dm, InfoCD cd, InfoPB pb, std::vector<InfoTB> tbs, std::vector<InfoFB> fbs) {
	printf("\n\nDevice ID: %s \nManufacturer: %s", pb.deviceId, pb.manName.c_str());
	printf("\n\nNumber of Physical Blocks: %d\n  Parent Class: %s\n   HW-Revision: %s \n   SW-Revision: %s", cd.numPB, pb.parentClass.c_str(), pb.hwRev, pb.swRev);
	printf("\n\nNumber of Transducer Blocks: %d", cd.numTB);
	for (int i = 0; i < tbs.size(); i++) {
		printf("\n  Parent Class: %s\n  Class: %s", tbs[i].parentClass.c_str(), tbs[i].childClass.c_str());
		for (int j = 0; j < tbs[i].numValues; j++) {
			printf("\n   %d. Value: %.3f %s\n   Flag: %.2hhX", j+1, tbs[i].value[j], tbs[i].unitName[j].c_str(), tbs[i].flag[j]);
		}
	}
	printf("\n\nNumber of Function Blocks: %d", cd.numFB);
	for (int i = 0; i < fbs.size(); i++) {
		printf("\n  Parent Class: %s\n  Class: %s", fbs[i].parentClass.c_str(), fbs[i].childClass.c_str());
		if (fbs[i].pcID == 1) {
			printf("\n   Value: %.3f %s\n   Flag: %.2hhX", fbs[i].value, fbs[i].unit.c_str(), fbs[i].flag);
		}
	}
	printf("\n\n");
	return 1;
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
	char bt[64];
	char pc[64];
	char cc[64];
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
	printf("\nGet Device Management...");
	numRcvdB = readparam(fla, add, slo, idx, response);
	if (numRcvdB == -1 || numRcvdB == 0) { return 0;}

	getVal(&numDirObj.bytes, response + 4, sizeof(numDirObj));
	getVal(&numDirEntry.bytes, response + 6, sizeof(numDirEntry));
	getVal(&numCompListDirEntry.bytes, response + 10, sizeof(numCompListDirEntry));
	getVal(&firstCompDirListDirEntry.bytes, response + 8, sizeof(firstCompDirListDirEntry));

	deviceManagement.numBTypes = numCompListDirEntry.uint;
	deviceManagement.numB = numDirEntry.uint - numCompListDirEntry.uint;

	//Directory List
	printf("\nGet Directory...");
	idx = firstCompDirListDirEntry.uint;
	numRcvdB = readparam(fla, add, slo, idx, response);
	if (numRcvdB == -1 || numRcvdB == 0) { return 0; }
	getVal(&value16, response + 2, sizeof(value16));
	getVal(&indexOffsetPB, response, sizeof(value16));
	compositeDirectory.numPB = value16.uint;
	getVal(&value16, response + 6, sizeof(value16));
	getVal(&indexOffsetTB, response + 4, sizeof(value16));
	compositeDirectory.numTB = value16.uint;
	getVal(&value16, response + 10, sizeof(value16));
	getVal(&indexOffsetFB, response + 8, sizeof(value16));
	compositeDirectory.numFB = value16.uint;

	/*
	 * Physical Block:
	 */
	printf("\nGet Physical Block...");
	if (indexOffsetPB.index != idx) {
		numRcvdB = readparam(fla, add, slo, indexOffsetPB.index, response);
		if (numRcvdB == -1 || numRcvdB == 0) { return 0; }
		indexOffsetPB.offset -= numCompListDirEntry.uint;
		indexOffsetTB.offset -= numCompListDirEntry.uint;
		indexOffsetFB.offset -= numCompListDirEntry.uint;
	}
	getVal(&slotIndexAddr, response + ((indexOffsetPB.offset - 1) * 4), sizeof(slotIndexAddr));
	if (address == 7) { slotIndexAddr.index = 114; } //Physical Block at another index as stated
	numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index, response);
	if (numRcvdB == -1) { return 0; }
	else if (numRcvdB) {
		memcpy(&value8, response + 1, 1);
		pbInfo.btID = value8;
		memcpy(&value8, response + 2, 1);
		pbInfo.pcID = value8;
		getBlock(pbInfo.btID, pbInfo.pcID, 0, bt, pc, cc);
		pbInfo.blockType = bt;
		pbInfo.parentClass = pc;
	}

	//Manufacturer
	numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 10, response);
	if (numRcvdB == -1) { return 0; }
	else if (numRcvdB) {
		getVal(&value16, response, sizeof(value16));
		pbInfo.manId = value16.uint;
		char s[64];
		getName(pbInfo.manId, s);
		pbInfo.manName = s;
	}
	//DeviceId
	numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 11, response);
	if (numRcvdB == -1) { return 0; }
	else if (numRcvdB) {
		memcpy(pbInfo.deviceId, response, sizeof(char)*numRcvdB);
		pbInfo.deviceId[16] = '\0';
	}
	//HW & SW Revision
	numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 9, response);
	if (numRcvdB == -1) { return 0; }
	else if (numRcvdB) {
		memcpy(pbInfo.hwRev, response, sizeof(char)*numRcvdB);
		pbInfo.hwRev[16] = '\0';
	}
	numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 8, response);
	if (numRcvdB == -1) { return 0; }
	else if (numRcvdB) {
		memcpy(pbInfo.swRev, response, sizeof(char)*numRcvdB);
		pbInfo.swRev[16] = '\0';
	}

	/*
	 *Transducer Blocks:
	 */
	printf("\nGet Transducer Blocks...");
	for (int i = 0; i < compositeDirectory.numTB; i++) {
		if (address == 7) { break; } //Transducer Block broken or at another address?
		InfoTB info{};
		numRcvdB = readparam(fla, add, slo, indexOffsetTB.index, response);
		if (numRcvdB == -1 || numRcvdB == 0) { return 0; }
		getVal(&slotIndexAddr, response + ((indexOffsetTB.offset - 1 + i) * 4), sizeof(slotIndexAddr));
		numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index, response);
		if (numRcvdB == -1) { return 0; }
		else if(numRcvdB){
			memcpy(&value8, response + 1, 1);
			info.btID = value8;
			memcpy(&value8, response + 2, 1);
			info.pcID = value8;
			memcpy(&value8, response + 3, 1);
			info.ccID = value8;
			getBlock(info.btID, info.pcID, info.ccID, bt, pc, cc);
			info.blockType = bt;
			info.parentClass = pc;
			info.childClass = cc;
			info.numValues = 0;
			switch (info.pcID) {
			case 1:
				numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 18, response);
				if (numRcvdB == -1) { return 0; }
				else if (numRcvdB) {
					getVal(&value101.bytes, response, 4);
					getVal(&value101.data.flag, response + 4, 1);
					info.value.push_back(value101.data.value);
					info.flag.push_back(value101.data.flag);
					info.numValues++;
				}
				numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 19, response);
				if (numRcvdB == -1) { return 0; }
				else if (numRcvdB) {
					getVal(&value16, response, sizeof(value16));
					info.unitId.push_back(value16.uint);
					getUnitName(value16.uint, reinterpret_cast<char*>(str));
					std::string s = reinterpret_cast<char*>(str);
					info.unitName.push_back(s);
				}
				if (info.ccID == 4) {
					numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 29, response);
					if (numRcvdB == -1) { return 0; }
					else if (numRcvdB) {
						getVal(&value101.bytes, response, 4);
						getVal(&value101.data.flag, response + 4, 1);
						info.value.push_back(value101.data.value);
						info.flag.push_back(value101.data.flag);
						info.numValues++;
					}
					numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 30, response);
					if (numRcvdB == -1) { return 0; }
					else if (numRcvdB) {
						getVal(&value16, response, sizeof(value16));
						info.unitId.push_back(value16.uint);
						getUnitName(value16.uint, reinterpret_cast<char*>(str));
						std::string s = reinterpret_cast<char*>(str);
						info.unitName.push_back(s);
					}
					numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 31, response);
					if (numRcvdB == -1) { return 0; }
					else if (numRcvdB) {
						getVal(&value101.bytes, response, 4);
						getVal(&value101.data.flag, response + 4, 1);
						info.value.push_back(value101.data.value);
						info.flag.push_back(value101.data.flag);
						info.numValues++;
					}
					numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 32, response);
					if (numRcvdB == -1) { return 0; }
					else if (numRcvdB) {
						getVal(&value16, response, sizeof(value16));
						info.unitId.push_back(value16.uint);
						getUnitName(value16.uint, reinterpret_cast<char*>(str));
						std::string s = reinterpret_cast<char*>(str);
						info.unitName.push_back(s);
					}
				}
				break;
			case 2:
				numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 8, response);
				if (numRcvdB == -1) { return 0; }
				else if (numRcvdB) {
					getVal(&value101.bytes, response, 4);
					getVal(&value101.data.flag, response + 4, 1);
					info.value.push_back(value101.data.value);
					info.flag.push_back(value101.data.flag);
					info.numValues++;
				}
				numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 9, response);
				if (numRcvdB == -1) { return 0; }
				else if (numRcvdB) {
					getVal(&value16, response, sizeof(value16));
					info.unitId.push_back(value16.uint);
					getUnitName(value16.uint, reinterpret_cast<char*>(str));
					std::string s = reinterpret_cast<char*>(str);
					info.unitName.push_back(s);
				}
				break;
			}
		}
		tbs.push_back(info);
	}

	/*
	 *Function Blocks:
	 */
	printf("\nGet Function Blocks...");
	for (int i = 0; i < compositeDirectory.numFB; i++) {
		if (address == 7 && i == compositeDirectory.numFB - 1) { break; } //Function Block broken or at another address?
		InfoFB info{};
		numRcvdB = readparam(fla, add, slo, indexOffsetFB.index, response);
		if (numRcvdB == -1 || numRcvdB == 0) { return 0; }
		getVal(&slotIndexAddr, response + ((indexOffsetFB.offset - 1 + i) * 4), sizeof(slotIndexAddr));
		numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index, response);
		if (numRcvdB == -1) { return 0; }
		else if (numRcvdB) {
			memcpy(&value8, response + 1, 1);
			info.btID = value8;
			memcpy(&value8, response + 2, 1);
			info.pcID = value8;
			memcpy(&value8, response + 3, 1);
			info.ccID = value8;
			getBlock(info.btID, info.pcID, info.ccID, bt, pc, cc);
			info.blockType = bt;
			info.parentClass = pc;
			info.childClass = cc;

			switch (info.pcID) {
			case 1: if (info.ccID == 1) {
				numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 10, response);
				if (numRcvdB == -1) { return 0; }
				else if (numRcvdB) {
					getVal(&value101.bytes, response, 4);
					info.value = value101.data.value;
					getVal(&value101.data.flag, response + 4, 1);
					info.flag = value101.data.flag;
				}
				numRcvdB = readparam(fla, add, slotIndexAddr.slot, slotIndexAddr.index + 35, response);
				if (numRcvdB == -1) { return 0; }
				else if (numRcvdB == 16) {
					memcpy(str, response, sizeof(char) * 16);
					info.unit = *str;
				}
			}
					break;
			}
		}
		fbs.push_back(info);
	}
	
	return printElements(deviceManagement, compositeDirectory, pbInfo, tbs, fbs);
}

int main(int argc, char **argv) {
	manIdTable.LoadFile("Man_ID_Table.xml");
	blockTypes.LoadFile("Block_Types.xml");

	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* Winsock DLL.                                  */
		printf("\nWSAStartup failed with error: %d", err);
		return 1;
	}

	/* Test if ID is in xml
	const char* name;
	int test = getName(123, name);
	*/


	
	uint16_t add = 6;

	while (true) {
		printf("\nPlease enter Address of Device: ");
		cin >> add;
		/*printf("\nSlot:");
		cin >> slo;
		printf("\nIndex:");
		cin >> idx;*/
		if (startRequest(add) == 1) {
		}
		else {
			printf("\nA problem occured, pleaes try again");
		}
	}
	
}