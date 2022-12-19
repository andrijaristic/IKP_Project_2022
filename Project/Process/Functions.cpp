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
        fprintf(stderr, "\tWRITE\033[0;31m select failed with error: %ld \033[0m\n", WSAGetLastError());
        return false;
    }

    if (iResult == 0)
    {
        return false;
    }

    return true;
}

bool IsSocketReadyForReading(SOCKET* socket, bool* connected) {
    FD_SET set;
    timeval timeVal;

    FD_ZERO(&set);
    FD_SET(*socket, &set);
    timeVal.tv_sec = 0;
    timeVal.tv_usec = 0;

    int iResult;

    iResult = select(0, &set, NULL, NULL, &timeVal);
    if (!*connected) {
        return false;
    }
    if (iResult == SOCKET_ERROR && *connected)
    {
        fprintf(stderr, "\tREAD\033[0;31m select failed with error: %ld \033[0m\n", WSAGetLastError());
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