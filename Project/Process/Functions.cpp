#include "Functions.h"
#include "stdio.h"

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

bool isSocketReadyForWriting(SOCKET* socket)
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
        fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
        return false;
    }

    if (iResult == 0)
    {
        return false;
    }

    return true;
}

bool isSocketReadyForReading(SOCKET* socket) {
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
        fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
        return false;
    }
    if (iResult == 0)
    {
        return false;
    }

    return true;
}