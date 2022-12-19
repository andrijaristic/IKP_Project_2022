#pragma once
#include <ws2tcpip.h>
#include "../Common/Message.h"

/// <summary>
/// Checks if socket still has established connection
/// </summary>
/// <param name="socket"></param>
/// <returns></returns>
bool IsSocketBroken(SOCKET socket);

/// <summary>
/// Calls select function on a socket and returns true if it is ready for writting operations, otherwise it returns false
/// </summary>
/// <param name="socket"></param>
/// <returns></returns>
bool IsSocketReadyForWriting(SOCKET* socket);

/// <summary>
/// Calls select function on a socket and returns true if it is ready for reading operations, otherwise it returns false
/// </summary>
/// <param name="socket"></param>
/// <param name="connected"></param>
/// <returns></returns>
bool IsSocketReadyForReading(SOCKET* socket, bool* connected);

/// <summary>
/// Attemps to send data to the given socket. Returns true if data sending is successful, false if it isn't
/// </summary>
/// <param name="socket"></param>
/// <param name="data"></param>
/// <returns></returns>
bool SendDataToReplicator(SOCKET* socket, MESSAGE* data);