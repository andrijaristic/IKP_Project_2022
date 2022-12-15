#include "Functions.h"
#include "stdio.h"
#include "../Common//Message.h"

bool SocketIsReadyForReading(SOCKET* socket)
{
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

bool SocketIsReadyForWriting(SOCKET* socket)
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

bool InitializeListenSocket(SOCKET* listenSocket, const char* port)
{
    int iResult;
    addrinfo* resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(NULL, port, &hints, &resultingAddress);
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        return false;
    }

    *listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (*listenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        return false;
    }

    iResult = bind(*listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        closesocket(*listenSocket);
        return false;
    }

    freeaddrinfo(resultingAddress);

    unsigned long int nonBlockingMode = 1;
    iResult = ioctlsocket(*listenSocket, FIONBIO, &nonBlockingMode);

    if (iResult == SOCKET_ERROR)
    {
        printf("ioctlsocket failed with error: %ld\n", WSAGetLastError());
        return false;
    }

    iResult = listen(*listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(*listenSocket);
        return false;
    }

    return true;
}

void RespondToProcessRegistration(SOCKET* acceptedSocket, bool registrationSuccessful)
{
    bool socketClosed = false;

    while (!SocketIsReadyForWriting(acceptedSocket))
    {
        if (IsSocketBroken(*acceptedSocket))
        {
            shutdown(*acceptedSocket, SD_BOTH);
            closesocket(*acceptedSocket);
            socketClosed = true;
            break;
        }
        Sleep(50);
    }
    
    if (socketClosed)
    {
        return;
    }

    int iResult;
    MESSAGE data;
    data.flag = registrationSuccessful ? REGISTRATION_SUCCESSFUL : REGISTRATION_FAILED;

    iResult = send(*acceptedSocket, (char*)&data, sizeof(data), 0);
    
    if (registrationSuccessful && (iResult != SOCKET_ERROR))
    {
        return;
    }

    shutdown(*acceptedSocket, SD_BOTH);
    closesocket(*acceptedSocket);
}

bool SendDataToReplicator(SOCKET* replicatorSocket, MESSAGE* data)
{
    while (!SocketIsReadyForWriting(replicatorSocket))
    {
        if (IsSocketBroken(*replicatorSocket))
        {
            return false;
        }
        Sleep(50);
    }

    int iResult = send(*replicatorSocket, (char*)&data, sizeof(data), 0);

    return iResult != SOCKET_ERROR;
}