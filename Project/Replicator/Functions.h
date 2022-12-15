#pragma once
#include <ws2tcpip.h>
#include "../Common//Message.h"

/// <summary>
/// Calls select on a socket and returns true if it is ready for reading operations, or false if it is not
/// </summary>
/// <param name="socket"></param>
/// <returns></returns>
bool SocketIsReadyForReading(SOCKET* socket);

/// <summary>
/// Calls select on a socket and returns true if it is ready for writing operations, or false if it is not
/// </summary>
/// <param name="socket"></param>
/// <returns></returns>
bool SocketIsReadyForWriting(SOCKET* socket);

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

/// <summary>
/// Informs process whether the registration process succeeded or failed
/// If it fails, ends the connection to the process
/// </summary>
/// <param name="acceptedSocket"></param>
/// <param name="registrationSuccessful"></param>
/// <returns></returns>
void RespondToProcessRegistration(SOCKET* acceptedSocket, bool registrationSuccessful);

/// <summary>
/// Attemps to send data to the given socket, if it succeeds return true, if it doesn't succeed return false
/// </summary>
/// <param name="replicatorSocket"></param>
/// <param name="data"></param>
/// <returns></returns>
bool SendDataToReplicator(SOCKET* replicatorSocket, MESSAGE* data);
