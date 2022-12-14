#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN

#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>

#include "Structures.h"
#include "Functions.h"

#pragma comment(lib, "Ws2_32.lib")

#define MAIN_REPLICATOR_PORT "28000"
#define THREAD_NUMBER 1
#define SAFE_DELETE_HANDLE(a)  if(a){CloseHandle(a);}

bool InitializeWindowsSockets();
DWORD WINAPI ConnectToMainReplicator(LPVOID param);
DWORD WINAPI AcceptReplicatorConnection(LPVOID param);

int main()
{
    if (InitializeWindowsSockets() == false)
    {
        return 1;
    }

    SOCKET listenSocket;
    SOCKET replicatorSocket;
    bool replicatorConnected = false;

    HANDLE FinishSignal = CreateSemaphore(0, 0, THREAD_NUMBER, NULL);
    DWORD replicatorConnectionThreadId;
    HANDLE replicatorConnection;

    char input[2];
    printf("Main replicator? (y/n): ");
    gets_s(input, 2);

    if (strcmp(input,"y") == 0)
    {
        if (!InitializeListenSocket(&listenSocket, MAIN_REPLICATOR_PORT))
        {
            WSACleanup();
            return 1;
        }

        printf("Server initialized, waiting for clients.\n");

        MAIN_REPLICATOR_DATA mainReplicatorData;
        mainReplicatorData.listenSocket = &listenSocket;
        mainReplicatorData.replicatorSocket = &replicatorSocket;
        mainReplicatorData.replicatorConnected = &replicatorConnected;
        mainReplicatorData.FinishSignal = &FinishSignal;

        replicatorConnection = CreateThread(NULL, 0, &AcceptReplicatorConnection, (LPVOID)&mainReplicatorData, 0, &replicatorConnectionThreadId);
    }
    else
    {
        replicatorSocket = socket(AF_INET,
            SOCK_STREAM,
            IPPROTO_TCP);

        if (replicatorSocket == INVALID_SOCKET)
        {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
        serverAddress.sin_port = htons(atoi(MAIN_REPLICATOR_PORT));

        SECONDARY_REPLICATOR_DATA secondaryReplicatorData;
        secondaryReplicatorData.replicatorSocket = &replicatorSocket;
        secondaryReplicatorData.replicatorConnected = &replicatorConnected;
        secondaryReplicatorData.serverAddress = &serverAddress;
        secondaryReplicatorData.FinishSignal = &FinishSignal;

        replicatorConnection = CreateThread(NULL, 0, &ConnectToMainReplicator, (LPVOID)&secondaryReplicatorData, 0, &replicatorConnectionThreadId);
    }

    printf("Press enter to terminate all threads\n");
    getchar();

    ReleaseSemaphore(FinishSignal, THREAD_NUMBER, NULL);
    if (replicatorConnection)
    {
        WaitForSingleObject(replicatorConnection, INFINITE);
    }

    SAFE_DELETE_HANDLE(replicatorConnection);
    SAFE_DELETE_HANDLE(FinishSignal);
    
    WSACleanup();
    printf("Everything cleaned, press enter to exit\n");
    getchar();
    return 0;
}

bool InitializeWindowsSockets()
{
    WSADATA wsaData;
    // Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

DWORD WINAPI ConnectToMainReplicator(LPVOID param)
{
    SECONDARY_REPLICATOR_DATA secondaryReplicatorData = *((SECONDARY_REPLICATOR_DATA*)param);
    SOCKET* replicatorSocket = secondaryReplicatorData.replicatorSocket;
    bool* replicatorConnected = secondaryReplicatorData.replicatorConnected;
    HANDLE* FinishSignal = secondaryReplicatorData.FinishSignal;
    sockaddr_in* serverAddress = secondaryReplicatorData.serverAddress;
    int iResult;

    while (WaitForSingleObject(*FinishSignal, 0) != WAIT_OBJECT_0)
    {
        if (*replicatorConnected)
        {
            if (!IsSocketBroken(*replicatorSocket))
            {
                Sleep(2000);
                continue;
            }
            printf("Connection to the replicator broken\n");
            closesocket(*replicatorSocket);
            *replicatorSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (*replicatorSocket == INVALID_SOCKET)
            {
                printf("socket failed with error: %ld\n", WSAGetLastError());
                WSACleanup();
                return 1;
            }
            *replicatorConnected = false;
        }

        iResult = connect(*replicatorSocket, (SOCKADDR*)serverAddress, sizeof(*serverAddress));
        if (iResult == SOCKET_ERROR)
        {
            Sleep(2000);
            continue;
        }
        printf("Connected to the replicator\n");

        unsigned long int nonBlockingMode = 1;
        iResult = ioctlsocket(*replicatorSocket, FIONBIO, &nonBlockingMode);

        if (iResult == SOCKET_ERROR)
        {
            printf("ioctlsocket failed with error: %ld\n", WSAGetLastError());
            return 1;
        }

        *replicatorConnected = true;
    }

    printf("ConnectToMainReplicator thread is shutting down\n");
    return 0;
}

DWORD WINAPI AcceptReplicatorConnection(LPVOID param)
{
    MAIN_REPLICATOR_DATA mainReplicatorData = *((MAIN_REPLICATOR_DATA*)param);
    SOCKET* listenSocket = mainReplicatorData.listenSocket;
    SOCKET* replicatorSocket = mainReplicatorData.replicatorSocket;
    bool* replicatorConnected = mainReplicatorData.replicatorConnected;
    HANDLE* FinishSignal = mainReplicatorData.FinishSignal;
    int iResult;

    while (WaitForSingleObject(*FinishSignal, 0) != WAIT_OBJECT_0)
    {
        if (*replicatorConnected)
        {
            if (!IsSocketBroken(*replicatorSocket))
            {
                Sleep(2000);
                continue;
            }
            printf("Connection to the replicator broken\n");
            closesocket(*replicatorSocket);
            *replicatorSocket = INVALID_SOCKET;
            *replicatorConnected = false;
        }

        if (!SocketIsReadyForReading(listenSocket))
        {
            Sleep(2000);
            continue;
        }

        *replicatorSocket = accept(*listenSocket, NULL, NULL);

        if (*replicatorSocket == INVALID_SOCKET)
        {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(*listenSocket);
            WSACleanup();
            return 1;
        }
        printf("Replicator connected\n");
        *replicatorConnected = true;
    }

    printf("AcceptReplicatorConnection thread is shutting down\n");
    return 0;
}