#pragma once
#include <ws2tcpip.h>

/// <summary>
/// Calls select on a socket and returns true if it is ready for reading operations, or false if it is not
/// </summary>
/// <param name="socket"></param>
/// <returns></returns>
bool SocketIsReadyForReading(SOCKET* socket);

/// <summary>
/// Checks if socket is broken
/// </summary>
/// <param name="socket"></param>
/// <returns></returns>
bool IsSocketBroken(SOCKET socket);

/// <summary>
/// Puts socket in a listening mode on the given port
/// Returns true if it succeeds or false if it doesn't
/// </summary>
/// <param name="listenSocket"></param>
/// <param name="port"></param>
/// <returns></returns>
bool InitializeListenSocket(SOCKET* listenSocket, const char* port);
