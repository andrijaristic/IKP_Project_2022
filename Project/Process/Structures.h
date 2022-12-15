#pragma once
#include <ws2tcpip.h>

#define MAX_PROCESS_ID_LENGTH 16

typedef struct REPLICATOR_DATA{
	HANDLE* FinishSignal;
	SOCKET* replicatorSocket;
	bool* replicatorConnected;
	sockaddr_in* serverAddress;
	char processId[MAX_PROCESS_ID_LENGTH];
} REPLICATOR_DATA;


typedef struct REPLICATOR_SEND_DATA {
	HANDLE* FinishSignal;
	SOCKET* replicatorSocket;
	char processId[MAX_PROCESS_ID_LENGTH];
	bool* shutdownSignal;
} REPLICATOR_SEND_DATA;