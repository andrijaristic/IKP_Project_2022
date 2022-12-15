#pragma once
#include <ws2tcpip.h>
#include "../Common//Message.h"
#include "../Common//HashMap.h"
#include "../Common//LinkedList.h"

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

typedef struct REPLICATOR_PROCESS_DATA {
	SOCKET* listenSocket;
	HashMap<SOCKET>* processSockets;
	HANDLE* FinishSignal;
}REPLICATOR_PROCESS_DATA;

typedef struct REPLICATOR_SENDER_DATA {
	SOCKET* replicatorSocket;
	bool* replicatorConnected;
	LinkedList<MESSAGE>* sendQueue;
	HANDLE* EmptySendQueue;
	HANDLE* FinishSignal;
}REPLICATOR_SENDER_DATA;

typedef struct REPLICATOR_RECEIVER_DATA {
	SOCKET* replicatorSocket;
	bool* replicatorConnected;
	LinkedList<MESSAGE>* recvQueue;
	HANDLE* EmptyRecvQueue;
	HANDLE* FinishSignal;
}REPLICATOR_RECEIVER_DATA;

typedef struct PROCESS_SENDER_DATA {
	HashMap<SOCKET>* processSockets;
	LinkedList<MESSAGE>* recvQueue;
	HANDLE* EmptyRecvQueue;
	HANDLE* FinishSignal;
}PROCESS_SENDER_DATA;