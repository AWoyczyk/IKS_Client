#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <iostream>
#include <stdio.h>

#pragma once

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
		printf("received %3d bytes\n", iRcvdBytes);
		if (buff[0] == sbuf[0]) {
			int i;
			for (i = 1; i < iRcvdBytes; i++) {
				printf(" %3d,", buff[i]);
			}
			printf("\n == \n");
			for (i = 1; i < iRcvdBytes; i++) {
				printf(" 0x%2X,", buff[i]);
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

int main(int argc, char **argv) {

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
	unsigned char fla = 0xff & rand();
	int add = 7;
	int slo = 1;
	int idx = 114;

	readparam(fla, add, slo, idx);
}