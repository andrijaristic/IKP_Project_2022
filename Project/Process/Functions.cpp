#include "Functions.h"
#include "stdio.h"
#include "../Common/Message.h"

bool IsSocketBroken(SOCKET socket)
{
    char buf;
    int err = recv(socket, &buf, 1, MSG_PEEK);
    if (err == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            return true;
        }
    }
    return false;
}

bool IsSocketReadyForWriting(SOCKET* socket)
{
    FD_SET set;
    timeval timeVal;

    FD_ZERO(&set);
    FD_SET(*socket, &set);

    timeVal.tv_sec = 0;
    timeVal.tv_usec = 0;

    int iResult;

    iResult = select(0, NULL, &set, NULL, &timeVal);

    if (iResult == SOCKET_ERROR)
    {
        fprintf(stderr, "WRITE select failed with error: %ld\n", WSAGetLastError());
        return false;
    }

    if (iResult == 0)
    {
        return false;
    }

    return true;
}

bool IsSocketReadyForReading(SOCKET* socket) {
    FD_SET set;
    timeval timeVal;

    FD_ZERO(&set);
    FD_SET(*socket, &set);
    timeVal.tv_sec = 0;
    timeVal.tv_usec = 0;

    int iResult;

    iResult = select(0, &set, NULL, NULL, &timeVal);
    if (iResult == SOCKET_ERROR)
    {
        fprintf(stderr, "READ select failed with error: %ld\n", WSAGetLastError());
        return false;
    }
    if (iResult == 0)
    {
        return false;
    }

    return true;
}

bool SendDataToReplicator(SOCKET* socket, MESSAGE* data) {
    while (!IsSocketReadyForWriting(socket)) {
        if (IsSocketBroken(*socket)) { return false; }
        Sleep(100);
    }

    int iResult = send(*socket, (char*)data, sizeof(*data), 0);

    return iResult != SOCKET_ERROR;
}