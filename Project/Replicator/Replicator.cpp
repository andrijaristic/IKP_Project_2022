#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN

#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>

#include "Structures.h"
#include "Functions.h"
#include "../Common//Message.h"
#include "../Common/HashMap.h"
#include "../Common//LinkedList.h"

#pragma comment(lib, "Ws2_32.lib")

#define MAIN_REPLICATOR_PORT "28000"
#define MAIN_REPLICATOR_PROCESS_PORT "27000"
#define SECONDARY_REPLICATOR_PROCESS_PORT "27001"
#define THREAD_NUMBER 6
#define SAFE_DELETE_HANDLE(a)  if(a){CloseHandle(a);}

bool InitializeWindowsSockets();
DWORD WINAPI ConnectToMainReplicator(LPVOID param);
DWORD WINAPI AcceptReplicatorConnection(LPVOID param);
DWORD WINAPI AcceptProcessConnections(LPVOID param);
DWORD WINAPI SendMessageToReplicator(LPVOID param);
DWORD WINAPI ReceiveMessageFromReplicator(LPVOID param);
DWORD WINAPI SendMessageToProcess(LPVOID param);
DWORD WINAPI ReceiveMessageFromProcess(LPVOID param);

int main()
{
    if (InitializeWindowsSockets() == false)
    {
        return 1;
    }

    SOCKET replicatorListenSocket;
    SOCKET replicatorSocket;
    bool replicatorConnected = false;

    HANDLE FinishSignal = CreateSemaphore(0, 0, THREAD_NUMBER, NULL);
    DWORD replicatorConnectionThreadId;
    HANDLE replicatorConnection;

    char input[2];
    printf("Main replicator? (y/n): ");
    gets_s(input, 2);
    bool isMainReplicator = strcmp(input, "y") == 0;

    if (isMainReplicator)
    {
        if (!InitializeListenSocket(&replicatorListenSocket, MAIN_REPLICATOR_PORT))
        {
            WSACleanup();
            return 1;
        }


        MAIN_REPLICATOR_DATA mainReplicatorData;
        mainReplicatorData.listenSocket = &replicatorListenSocket;
        mainReplicatorData.replicatorSocket = &replicatorSocket;
        mainReplicatorData.replicatorConnected = &replicatorConnected;
        mainReplicatorData.FinishSignal = &FinishSignal;

        replicatorConnection = CreateThread(NULL, 0, &AcceptReplicatorConnection, (LPVOID)&mainReplicatorData, 0, &replicatorConnectionThreadId);

        printf("Listening to other replicators.\n");
    }
    else
    {
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

    SOCKET processListenSocket;
    DWORD processConnectionThreadId;
    HANDLE processConnection;
    HashMap<SOCKET> processSockets;

    if (!InitializeListenSocket(&processListenSocket, isMainReplicator ? MAIN_REPLICATOR_PROCESS_PORT : SECONDARY_REPLICATOR_PROCESS_PORT))
    {
        WSACleanup();
        return 1;
    }

    REPLICATOR_PROCESS_DATA replicatorProcessData;
    replicatorProcessData.listenSocket = &processListenSocket;
    replicatorProcessData.processSockets = &processSockets;
    replicatorProcessData.FinishSignal = &FinishSignal;

    processConnection = CreateThread(NULL, 0, &AcceptProcessConnections, (LPVOID)&replicatorProcessData, 0, &processConnectionThreadId);

    printf("Listening to processes.\n");

    HANDLE EmptySendQueue = CreateSemaphore(0, 0, 1, NULL);
    LinkedList<MESSAGE> sendQueue;
    DWORD replicatorSenderThreadId;
    HANDLE replicatorSender;

    REPLICATOR_SENDER_DATA replicatorSenderData;
    replicatorSenderData.replicatorSocket = &replicatorSocket;
    replicatorSenderData.replicatorConnected = &replicatorConnected;
    replicatorSenderData.sendQueue = &sendQueue;
    replicatorSenderData.EmptySendQueue = &EmptySendQueue;
    replicatorSenderData.FinishSignal = &FinishSignal;

    replicatorSender = CreateThread(NULL, 0, &SendMessageToReplicator, (LPVOID)&replicatorSenderData, 0, &replicatorSenderThreadId);

    HANDLE EmptyRecvQueue = CreateSemaphore(0, 0, 1, NULL);
    LinkedList<MESSAGE> recvQueue;
    DWORD replicatorReceiverThreadId;
    HANDLE replicatorReceiver;

    REPLICATOR_RECEIVER_DATA replicatorReceiverData;
    replicatorReceiverData.replicatorSocket = &replicatorSocket;
    replicatorReceiverData.replicatorConnected = &replicatorConnected;
    replicatorReceiverData.recvQueue = &recvQueue;
    replicatorReceiverData.EmptyRecvQueue = &EmptyRecvQueue;
    replicatorReceiverData.FinishSignal = &FinishSignal;

    replicatorReceiver = CreateThread(NULL, 0, &ReceiveMessageFromReplicator, (LPVOID)&replicatorReceiverData, 0, &replicatorReceiverThreadId);

    DWORD processSenderThreadId;
    HANDLE processSender;

    PROCESS_SENDER_DATA processSenderData;
    processSenderData.processSockets = &processSockets;
    processSenderData.recvQueue = &recvQueue;
    processSenderData.EmptyRecvQueue = &EmptyRecvQueue;
    processSenderData.FinishSignal = &FinishSignal;

    processSender = CreateThread(NULL, 0, &SendMessageToProcess, (LPVOID)&processSenderData, 0, &processSenderThreadId);

    DWORD processReceiverThreadId;
    HANDLE processReceiver;

    PROCESS_RECEIVER_DATA processReceiverData;
    processReceiverData.processSockets = &processSockets;
    processReceiverData.sendQueue = &sendQueue;
    processReceiverData.EmptySendQueue = &EmptySendQueue;
    processReceiverData.FinishSignal = &FinishSignal;

    processReceiver = CreateThread(NULL, 0, &ReceiveMessageFromProcess, (LPVOID)&processReceiverData, 0, &processReceiverThreadId);

    printf("Press enter to terminate all threads\n");
    getchar();

    ReleaseSemaphore(FinishSignal, THREAD_NUMBER, NULL);

    if (replicatorConnection)
    {
        WaitForSingleObject(replicatorConnection, INFINITE);
    }
    if (processConnection)
    {
        WaitForSingleObject(processConnection, INFINITE);
    }
    if (replicatorSender)
    {
        WaitForSingleObject(replicatorSender, INFINITE);
    }
    if (replicatorReceiver)
    {
        WaitForSingleObject(replicatorReceiver, INFINITE);
    }
    if (processSender)
    {
        WaitForSingleObject(processSender, INFINITE);
    }
    if (processReceiver)
    {
        WaitForSingleObject(processReceiver, INFINITE);
    }

    SAFE_DELETE_HANDLE(replicatorConnection);
    SAFE_DELETE_HANDLE(processConnection);
    SAFE_DELETE_HANDLE(replicatorSender);
    SAFE_DELETE_HANDLE(replicatorReceiver);
    SAFE_DELETE_HANDLE(processSender);
    SAFE_DELETE_HANDLE(processReceiver);
    SAFE_DELETE_HANDLE(FinishSignal);
    SAFE_DELETE_HANDLE(EmptySendQueue);
    SAFE_DELETE_HANDLE(EmptyRecvQueue);

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
            shutdown(*replicatorSocket, SD_BOTH);
            closesocket(*replicatorSocket);
            *replicatorSocket = INVALID_SOCKET;
            *replicatorConnected = false;
        }

        *replicatorSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (*replicatorSocket == INVALID_SOCKET)
        {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
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
            shutdown(*replicatorSocket, SD_BOTH);
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

DWORD WINAPI AcceptProcessConnections(LPVOID param)
{
    REPLICATOR_PROCESS_DATA replicatorProcessData = *((REPLICATOR_PROCESS_DATA*)param);
    SOCKET* listenSocket = replicatorProcessData.listenSocket;
    HashMap<SOCKET>* processSockets = replicatorProcessData.processSockets;
    HANDLE* FinishSignal = replicatorProcessData.FinishSignal;


    int iResult;
    SOCKET acceptedSocket;

    while (WaitForSingleObject(*FinishSignal, 0) != WAIT_OBJECT_0)
    {
        if (!SocketIsReadyForReading(listenSocket))
        {
            Sleep(2000);
            continue;
        }

        acceptedSocket = accept(*listenSocket, NULL, NULL);

        if (acceptedSocket == INVALID_SOCKET)
        {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(*listenSocket);
            WSACleanup();
            return 1;
        }
        printf("Process connected, waiting for registration\n");

        bool socketReadyToReceive = true;
        while (!SocketIsReadyForReading(&acceptedSocket))
        {
            if (IsSocketBroken(acceptedSocket))
            {
                shutdown(acceptedSocket, SD_BOTH);
                closesocket(acceptedSocket);
                socketReadyToReceive = false;
                break;
            }
            Sleep(50);
        }

        // socket was broken and closed
        if (!socketReadyToReceive)
        {
            continue;
        }

        char recvBuffer[sizeof(MESSAGE)];
        iResult = recv(acceptedSocket, recvBuffer, sizeof(MESSAGE), 0);
        if (iResult == 0)
        {
            printf("Connection with process closed\n");
            shutdown(acceptedSocket, SD_BOTH);
            closesocket(acceptedSocket);
        }
        else if (iResult == SOCKET_ERROR)
        {
            //recv failed
            printf("recv failed with error %d\n", WSAGetLastError());
            shutdown(acceptedSocket, SD_BOTH);
            closesocket(acceptedSocket);
        }
        else
        {
            // message received
            MESSAGE* message = (MESSAGE*)recvBuffer;
            if (message->flag != REGISTRATION)
            {
                RespondToProcessRegistration(&acceptedSocket, false);
                continue;
            }

            char processId[MAX_PROCESS_ID_LENGTH];
            strcpy_s(processId, message->processId);

            bool processExists = processSockets->ContainsKey(processId);

            if (processExists)
            {
                RespondToProcessRegistration(&acceptedSocket, false);
                continue;
            }

            printf("Process registration successful, processId: %s\n", processId);
            RespondToProcessRegistration(&acceptedSocket, true);
            processSockets->Insert(processId, acceptedSocket);
        }
    }

    printf("AcceptProcessConnections thread is shutting down\n");
    return 0;
}

DWORD WINAPI SendMessageToReplicator(LPVOID param)
{
    REPLICATOR_SENDER_DATA replicatorSenderData = *((REPLICATOR_SENDER_DATA*)param);
    SOCKET* replicatorSocket = replicatorSenderData.replicatorSocket;
    bool* replicatorConnected = replicatorSenderData.replicatorConnected;
    LinkedList<MESSAGE>* sendQueue = replicatorSenderData.sendQueue;
    HANDLE* EmptySendQueue = replicatorSenderData.EmptySendQueue;
    HANDLE* FinishSignal = replicatorSenderData.FinishSignal;

    const int semaphoreNum = 2;
    HANDLE semaphores[semaphoreNum] = { *FinishSignal, *EmptySendQueue };

    while (WaitForMultipleObjects(semaphoreNum, semaphores, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
    {
        if (!*replicatorConnected)
        {
            ReleaseSemaphore(*EmptySendQueue, 1, NULL);
            Sleep(1000);
            continue;
        }

        MESSAGE data;
        if (!sendQueue->PopFront(&data))
        {
            //printf("Could not get the message from the send queue\n");
            Sleep(1000);
            continue;
        }

        if (!SendDataToSocket(replicatorSocket, &data))
        {
            printf("Failed to send message to other replicator\n");
            shutdown(*replicatorSocket, SD_BOTH);
            closesocket(*replicatorSocket);
            *replicatorConnected = false;
            sendQueue->PushFront(data);
        }

        if (sendQueue->isEmpty())
        {
            continue;
        }
        ReleaseSemaphore(*EmptySendQueue, 1, NULL);
    }

    printf("SendMessageToReplicator thread is shutting down\n");
    return 0;
}

DWORD WINAPI ReceiveMessageFromReplicator(LPVOID param)
{
    REPLICATOR_RECEIVER_DATA replicatorRecvData = *((REPLICATOR_RECEIVER_DATA*)param);
    SOCKET* replicatorSocket = replicatorRecvData.replicatorSocket;
    bool* replicatorConnected = replicatorRecvData.replicatorConnected;
    LinkedList<MESSAGE>* recvQueue = replicatorRecvData.recvQueue;
    HANDLE* EmptyRecvQueue = replicatorRecvData.EmptyRecvQueue;
    HANDLE* FinishSignal = replicatorRecvData.FinishSignal;

    int iResult;
    char recvBuffer[sizeof(MESSAGE)];

    while (WaitForSingleObject(*FinishSignal, 0) != WAIT_OBJECT_0)
    {
        if (!*replicatorConnected)
        {
            Sleep(1000);
            continue;
        }

        if (IsSocketBroken(*replicatorSocket))
        {
            shutdown(*replicatorSocket, SD_BOTH);
            closesocket(*replicatorSocket);
            *replicatorConnected = false;
            continue;
        }

        if (!SocketIsReadyForReading(replicatorSocket))
        {
            Sleep(1000);
            continue;
        }

        memset(recvBuffer, 0, sizeof(recvBuffer));
        int bytesReceived = 0;
        while (bytesReceived != sizeof(MESSAGE))
        {
            iResult = recv(*replicatorSocket, recvBuffer + bytesReceived, sizeof(MESSAGE) - bytesReceived, 0);
            if (iResult == 0)
            {
                printf("Connection with replicator closed\n");
                shutdown(*replicatorSocket, SD_BOTH);
                closesocket(*replicatorSocket);
                *replicatorConnected = false;
            }
            else if (iResult == SOCKET_ERROR)
            {
                if (WSAGetLastError() == WSAEWOULDBLOCK)
                {
                    continue;
                }
                //recv failed
                printf("recv failed with error %d\n", WSAGetLastError());
                shutdown(*replicatorSocket, SD_BOTH);
                closesocket(*replicatorSocket);
                *replicatorConnected = false;
                break;
            }
            else
            {
                bytesReceived += iResult;
                if (bytesReceived != sizeof(MESSAGE))
                {
                    continue;
                }
                // message received
                MESSAGE* message = (MESSAGE*)recvBuffer;
                if (message->flag != DATA)
                {
                    break;
                }
                recvQueue->PushBack(*message);
                ReleaseSemaphore(*EmptyRecvQueue, 1, NULL);
            }
        }
        
    }

    printf("ReceiveMessageFromReplicator thread is shutting down\n");
    return 0;
}

DWORD WINAPI SendMessageToProcess(LPVOID param)
{
    PROCESS_SENDER_DATA processSendData = *((PROCESS_SENDER_DATA*)param);
    HashMap<SOCKET>* processSockets = processSendData.processSockets;
    LinkedList<MESSAGE>* recvQueue = processSendData.recvQueue;
    HANDLE* EmptyRecvQueue = processSendData.EmptyRecvQueue;
    HANDLE* FinishSignal = processSendData.FinishSignal;

    const int semaphoreNum = 2;
    HANDLE semaphores[semaphoreNum] = { *FinishSignal, *EmptyRecvQueue };

    while (WaitForMultipleObjects(semaphoreNum, semaphores, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
    {
        MESSAGE data;
        if (!recvQueue->PopFront(&data))
        {
            //printf("Could not get the message from the recv queue\n");
            Sleep(1000);
            continue;
        }
        char processId[MAX_PROCESS_ID_LENGTH];
        strcpy_s(processId, data.processId);

        bool processConnected = processSockets->ContainsKey(processId);
        if (!processConnected)
        {
            recvQueue->PushBack(data);
            ReleaseSemaphore(*EmptyRecvQueue, 1, NULL);
            Sleep(50);
            continue;
        }

        SOCKET acceptedSocket;
        processSockets->Get(processId, &acceptedSocket);

        if (!SendDataToSocket(&acceptedSocket, &data))
        {
            printf("Failed to send message to the process\n");
            recvQueue->PushBack(data);
            ReleaseSemaphore(*EmptyRecvQueue, 1, NULL);
            shutdown(acceptedSocket, SD_BOTH);
            closesocket(acceptedSocket);
            processSockets->Delete(processId);
            continue;
        }

        if (recvQueue->Count() == 0)
        {
            continue;
        }
        ReleaseSemaphore(*EmptyRecvQueue, 1, NULL);
    }

    printf("SendMessageToProcess thread is shutting down\n");
    return 0;
}


DWORD WINAPI ReceiveMessageFromProcess(LPVOID param)
{
    PROCESS_RECEIVER_DATA processRecvData = *((PROCESS_RECEIVER_DATA*)param);
    HashMap<SOCKET>* processSockets = processRecvData.processSockets;
    LinkedList<MESSAGE>* sendQueue = processRecvData.sendQueue;
    HANDLE* EmptySendQueue = processRecvData.EmptySendQueue;
    HANDLE* FinishSignal = processRecvData.FinishSignal;

    int iResult;
    char recvBuffer[sizeof(MESSAGE)];

    while (WaitForSingleObject(*FinishSignal, 0) != WAIT_OBJECT_0)
    {
        int socketCount = 0;
        int keyCount = 0;
        SOCKET* sockets = processSockets->GetValues(&socketCount);
        char** keys = processSockets->GetKeys(&keyCount);
        
        if (sockets == NULL || keys==NULL || (socketCount != keyCount))
        {
            if (keys != NULL)
            {
                for (int i = 0; i < keyCount; i++)
                {
                    free(keys[i]);
                }
                free(keys);
            }
            
            if (sockets != NULL)
            {
                free(sockets);
            }
            Sleep(1000);
            continue;
        }

        for (int i = 0; i < socketCount; i++)
        {
            if (!SocketIsReadyForReading(sockets + i))
            {
                Sleep(50);
                continue;
            }
            int bytesReceived = 0;
            memset(recvBuffer, 0, sizeof(recvBuffer));
            while (bytesReceived != sizeof(MESSAGE))
            {
                iResult = recv(sockets[i], recvBuffer + bytesReceived, sizeof(MESSAGE) - bytesReceived, 0);
                if (iResult == 0)
                {
                    printf("Connection with process closed\n");
                    shutdown(sockets[i], SD_BOTH);
                    closesocket(sockets[i]);
                    // find socket and remove it from hashmap
                    processSockets->Delete(keys[i]);
                }
                else if (iResult == SOCKET_ERROR)
                {
                    if (WSAGetLastError() == WSAEWOULDBLOCK)
                    {
                        continue;
                    }
                    //recv failed
                    printf("recv failed with error %d\n", WSAGetLastError());
                    shutdown(sockets[i], SD_BOTH);
                    closesocket(sockets[i]);
                    // find socket and remove it from hashmap
                    processSockets->Delete(keys[i]);
                    break;
                }
                else
                {
                    bytesReceived += iResult;
                    if (bytesReceived != sizeof(MESSAGE))
                    {
                        continue;
                    }
                    // message received
                    MESSAGE* message = (MESSAGE*)recvBuffer;
                    if (message->flag != DATA)
                    {
                        break;
                    }
                    sendQueue->PushBack(*message);
                    ReleaseSemaphore(*EmptySendQueue, 1, NULL);
                }
            }
        }

        for (int i = 0; i < keyCount; i++)
        {
            free(keys[i]);
        }
        free(keys);
        free(sockets);
    }

    printf("ReceiveMessageFromProcess thread is shutting down\n");
    return 0;
}