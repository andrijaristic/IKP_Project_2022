#pragma once
#include <ws2tcpip.h>

bool IsSocketBroken(SOCKET socket);
bool isSocketReadyForWriting(SOCKET* socket);
bool isSocketReadyForReading(SOCKET* socket);