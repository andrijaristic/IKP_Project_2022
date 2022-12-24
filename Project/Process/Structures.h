#pragma once
#include <ws2tcpip.h>

#define MAX_PROCESS_ID_LENGTH 16

typedef struct REPLICATOR_DATA{
	HANDLE* FinishSignal;
	SOCKET* replicatorSocket;
	bool* replicatorConnected;
	bool* registrationSuccessful;
	sockaddr_in* serverAddress;
	char* processId;
} REPLICATOR_DATA;

typedef struct REPLICATOR_SEND_DATA {
	HANDLE* FinishSignal;
	SOCKET* replicatorSocket;
	char processId[MAX_PROCESS_ID_LENGTH];
	bool* shutdownSignal;
	bool* replicatorConnected;
	bool* registrationSuccessful;
} REPLICATOR_SEND_DATA;

typedef struct REPLICATOR_RECEIVE_DATA {
	HANDLE* FinishSignal;
	SOCKET* replicatorSocket;
	char processId[MAX_PROCESS_ID_LENGTH];
	bool* replicatorConnected;
	bool* registrationSuccessful;
	bool stressTest;
} REPLICATOR_RECEIVE_DATA;
