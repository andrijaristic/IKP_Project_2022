#pragma once
#include <ws2tcpip.h>

typedef struct REPLICATOR_DATA{
	HANDLE* FinishSignal;
	SOCKET* replicatorSocket;
	bool* replicatorConnected;
	sockaddr_in* serverAddress;
	short processId;
} REPLICATOR_DATA;