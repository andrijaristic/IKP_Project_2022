#pragma once
#include <ws2tcpip.h>

bool SocketIsReadyForReading(SOCKET* socket);

bool IsSocketBroken(SOCKET socket);

bool InitializeListenSocket(SOCKET* listenSocket, const char* port);