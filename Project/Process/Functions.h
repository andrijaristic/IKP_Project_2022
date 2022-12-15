#pragma once
#include <ws2tcpip.h>
#include "../Common/Message.h"

bool IsSocketBroken(SOCKET socket);
bool IsSocketReadyForWriting(SOCKET* socket);
bool IsSocketReadyForReading(SOCKET* socket);
bool SendDataToReplicator(SOCKET* socket, MESSAGE* data);