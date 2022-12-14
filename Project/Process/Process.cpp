#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN

#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>

#include "Functions.h"
#include "Structures.h";
#include "../Common/Message.h"

#pragma comment(lib, "Ws2_32.lib")

#define SAFE_DELETE_HANDLE(a)  if(a){CloseHandle(a);}
#define MAIN_REPLICATOR_PORT "27000"
#define SECONDARY_REPLICATOR_PORT "27001"
#define THREAD_NUMBER 1
#define MAX_BUFFER_LENGTH 512

bool InitializeWindowsSockets();
DWORD WINAPI ConnectToReplicator(LPVOID param);

int main()
{
    if (InitializeWindowsSockets() == false)
    {
        return 1;
    }

    SOCKET replicatorSocket;
    bool replicatorConnected = false;
    HANDLE FinishSignal = CreateSemaphore(0, 0, THREAD_NUMBER, NULL);
    DWORD replicatorConnectionThreadId;
    HANDLE replicatorConnection;

    short processId = 0;
    do {
        printf("Enter ProcessID: ");
        scanf_s("%hd", &processId);
    } while (processId <= 0);

    short replicatorPort = 0;
    printf("\nChoose Replicator to connect to\n---------------------------------------------\n");
    printf("1. Main Replicator (27000)\n");
    printf("2. Secondary Replicator (27001)\n---------------------------------------------\n");
    do {
        printf("Pick option: ");
        scanf_s("%hd", &replicatorPort);
    } while (replicatorPort <= 0 || replicatorPort >= 3);

    printf("\nClient ProcessID: %d\nConnected Replicator Port: %d\n\n", processId, replicatorPort);

    replicatorPort = replicatorPort == 1 ? htons(atoi(MAIN_REPLICATOR_PORT)) : htons(atoi(SECONDARY_REPLICATOR_PORT));

    // Making socket for process to replicator connection.
    replicatorSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (replicatorSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = replicatorPort;

    REPLICATOR_DATA replicatorData;
    replicatorData.replicatorSocket = &replicatorSocket;
    replicatorData.serverAddress = &serverAddress;
    replicatorData.FinishSignal = &FinishSignal;
    replicatorData.replicatorConnected = &replicatorConnected;
    replicatorData.processId = processId;

    // Actual connection logic. Make it so it's unblocking and constantly trying to connect to the replicator.
    replicatorConnection = CreateThread(NULL, 0, &ConnectToReplicator, (LPVOID)&replicatorData, 0, &replicatorConnectionThreadId);

    // During connection establishment, enable posting messages which will be stored in an internal buffer or list.

    printf("Press Enter to terminate all threads.\n");
    getchar();
    getchar();

    ReleaseSemaphore(FinishSignal, THREAD_NUMBER, NULL);
    if (replicatorConnected) {
        WaitForSingleObject(replicatorConnection, INFINITE);
    }

    SAFE_DELETE_HANDLE(replicatorConnection);
    SAFE_DELETE_HANDLE(FinishSignal);

    WSACleanup();
    printf("Cleanup finished.\nPress Enter to exit.\n");
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

DWORD WINAPI ConnectToReplicator(LPVOID param) {
    REPLICATOR_DATA replicatorData = *((REPLICATOR_DATA*)param);
    HANDLE* FinishSignal = replicatorData.FinishSignal;
    SOCKET* replicatorSocket = replicatorData.replicatorSocket;
    bool* replicatorConnected = replicatorData.replicatorConnected;
    sockaddr_in* serverAddress = replicatorData.serverAddress;
    short processId = replicatorData.processId;
    int iResult;

    int onConnectMessage = 0;

    // Waiting for Semaphore to release Thread connected to Replicator for Graceful Shutdown reasons.
    while (WaitForSingleObject(*FinishSignal, 0) != WAIT_OBJECT_0) {
        if (*replicatorConnected) {
            if (!IsSocketBroken(*replicatorSocket)) {
                Sleep(2000);
                continue;
            }

            printf("Connection to the Replicator has been broken.\n");
            closesocket(*replicatorSocket);

            *replicatorSocket = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
            if (*replicatorSocket == INVALID_SOCKET) {
                printf("socket failed with error: %ld\n", WSAGetLastError());
                WSACleanup();
                return 1;
            }

            *replicatorConnected = true;
        }

        iResult = connect(*replicatorSocket, (SOCKADDR*)serverAddress, sizeof(*serverAddress));
        if (iResult == SOCKET_ERROR) {
            Sleep(2000);
            continue;
        }

        printf("Successfully connected to Replicator.\n");

        unsigned long int nonBlockingMode = 1;
        iResult = ioctlsocket(*replicatorSocket, FIONBIO, &nonBlockingMode);
        if (iResult == SOCKET_ERROR) {
            printf("ioctlsocket failed with error: %ld\n", WSAGetLastError());
            return 1;
        }

        *replicatorConnected = true;

        if (!onConnectMessage) {
            struct MESSAGE data;
            data.processId = processId;
            data.flag = DATA;
            strcpy_s(data.message, "");

            while (!isSocketReady(replicatorSocket)) {
                printf("Cannot send message to replicator.\n");
                Sleep(1000);
            }

            iResult = send(*replicatorSocket, (char*)&data, sizeof(data), 0);
            if (iResult == SOCKET_ERROR) {
                closesocket(*replicatorSocket);
                WSACleanup();
                return 1;
            }

            onConnectMessage = 1;
        }
    }
}