#pragma once
#include <ws2tcpip.h>

bool IsSocketBroken(SOCKET socket);
bool isSocketReady(SOCKET* socket);