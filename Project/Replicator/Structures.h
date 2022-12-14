#pragma once
#include <ws2tcpip.h>

typedef struct SECONDARY_REPLICATOR_DATA {
	SOCKET* replicatorSocket;
	bool* replicatorConnected;
	HANDLE* FinishSignal;
	sockaddr_in* serverAddress;
}SECONDARY_REPLICATOR_DATA;

typedef struct MAIN_REPLICATOR_DATA {
	SOCKET* listenSocket;
	SOCKET* replicatorSocket;
	bool* replicatorConnected;
	HANDLE* FinishSignal;
}MAIN_REPLICATOR_DATA;