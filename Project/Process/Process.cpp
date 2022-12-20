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

#define THREAD_NUMBER 3
#define DEFAULT_BUFFER_LENGTH 1024
#define MAX_PROCESS_ID_LENGTH 16

bool InitializeWindowsSockets();
DWORD WINAPI ConnectToReplicator(LPVOID param);
DWORD WINAPI SendMessageToReplicator(LPVOID param);
DWORD WINAPI ReceiveMessageFromReplicator(LPVOID param);

int main()
{
    if (InitializeWindowsSockets() == false)
    {
        return 1;
    }

    SOCKET replicatorSocket;
    bool replicatorConnected = false;
    bool registrationSuccessful = false;
    HANDLE FinishSignal = CreateSemaphore(0, 0, THREAD_NUMBER, NULL);

    DWORD replicatorConnectionThreadId;
    HANDLE replicatorConnection;

    char processId[MAX_PROCESS_ID_LENGTH];
    do {
        printf("Enter ProcessID: ");
        gets_s(processId, MAX_PROCESS_ID_LENGTH);
    } while (atoi(processId) <= 0);

    short replicatorPort = 0;
    printf("\nChoose Replicator to connect to\n---------------------------------------------\n");
    printf("1. Main Replicator (27000)\n");
    printf("2. Secondary Replicator (27001)\n---------------------------------------------\n");
    do {
        printf("Pick option: ");
        scanf_s("%hd", &replicatorPort);
    } while (replicatorPort <= 0 || replicatorPort >= 3);
    getchar(); // Flushing leftover newline.
    printf("\nClient ProcessID: %s\nConnected Replicator Port: %d\n\n", processId, replicatorPort == 1 ? 27000 : 27001);

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
    replicatorData.registrationSuccessful = &registrationSuccessful;
    replicatorData.processId = processId;

    replicatorConnection = CreateThread(NULL, 0, &ConnectToReplicator, (LPVOID)&replicatorData, 0, &replicatorConnectionThreadId);

    while (!registrationSuccessful) {
        continue;
    }

    bool shutdownSignal = false;
    DWORD senderThreadId, receiverThreadId;
    HANDLE senderThread, receiverThread;

    REPLICATOR_SEND_DATA replicatorSendData;
    replicatorSendData.FinishSignal = &FinishSignal;
    replicatorSendData.replicatorSocket = &replicatorSocket;
    replicatorSendData.shutdownSignal = &shutdownSignal;
    replicatorSendData.replicatorConnected = &replicatorConnected;
    replicatorSendData.registrationSuccessful = &registrationSuccessful;
    strcpy_s(replicatorSendData.processId, processId);

    senderThread = CreateThread(NULL, 0, &SendMessageToReplicator, (LPVOID)&replicatorSendData, 0, &senderThreadId);

    REPLICATOR_RECEIVE_DATA replicatorReceiveData;
    replicatorReceiveData.FinishSignal = &FinishSignal;
    replicatorReceiveData.replicatorSocket = &replicatorSocket;
    replicatorReceiveData.replicatorConnected = &replicatorConnected;
    replicatorReceiveData.registrationSuccessful = &registrationSuccessful;
    strcpy_s(replicatorReceiveData.processId, processId);

    receiverThread = CreateThread(NULL, 0, &ReceiveMessageFromReplicator, (LPVOID)&replicatorReceiveData, 0, &receiverThreadId);

    while (!shutdownSignal) {
        continue;
    }

    ReleaseSemaphore(FinishSignal, THREAD_NUMBER, NULL);
    if (replicatorConnection) {
        WaitForSingleObject(replicatorConnection, INFINITE);
    }
    if (senderThread) {
        WaitForSingleObject(senderThread, INFINITE);
    }
    if (receiverThread) {
        WaitForSingleObject(receiverThread, INFINITE);
    }

    SAFE_DELETE_HANDLE(FinishSignal);
    SAFE_DELETE_HANDLE(senderThread);
    SAFE_DELETE_HANDLE(receiverThread);

    WSACleanup();
    printf("\nCleanup finished.\nPress Enter to exit.\n");
    getchar();

    return 0;
}

DWORD WINAPI SendMessageToReplicator(LPVOID param) {
    REPLICATOR_SEND_DATA replicatorSendData = *((REPLICATOR_SEND_DATA*)param);
    HANDLE* FinishSignal = replicatorSendData.FinishSignal;
    SOCKET* replicatorSocket = replicatorSendData.replicatorSocket;
    bool* shutdownSignal = replicatorSendData.shutdownSignal;
    bool* replicatorConnected = replicatorSendData.replicatorConnected;
    bool* registrationSuccessful = replicatorSendData.registrationSuccessful;
    char processId[MAX_PROCESS_ID_LENGTH];
    strcpy_s(processId, replicatorSendData.processId);

    char message[DEFAULT_BUFFER_LENGTH];
    printf("\nMESSAGE SENDING ENABLED\n");
    printf("\n'exit' for application shut down\n");
    printf("-----------------------------\n");
    while (WaitForSingleObject(*FinishSignal, 0) != WAIT_OBJECT_0) {
        // Check for shutdownSignal so there's no lingering gets_s() running while cleanup is happening for smoother UX
        if (!*shutdownSignal){
            printf("MESSAGE: ");
            gets_s(message, DEFAULT_BUFFER_LENGTH);

            if (!strcmp(message, "exit")) {
                shutdown(*replicatorSocket, SD_BOTH);
                closesocket(*replicatorSocket);

                *shutdownSignal = true;
            }

            if (!*shutdownSignal && *replicatorConnected) {
                MESSAGE data;
                strcpy_s(data.processId, processId);
                strcpy_s(data.message, message);
                data.flag = DATA;

                if (!SendDataToReplicator(replicatorSocket, &data)) {
                    printf("Failed to send data to replicator.");
                    shutdown(*replicatorSocket, SD_BOTH);
                    closesocket(*replicatorSocket);
                }
            }
            else if (!*shutdownSignal && !*replicatorConnected) {
                printf("\nNot connected to replicator.\n");
            }
        }
    }

    printf("SendMessageToReplicator Thread is shutting down.\n");
    return 0;
}

DWORD WINAPI ReceiveMessageFromReplicator(LPVOID param) {
    REPLICATOR_RECEIVE_DATA replicatorReceiveData = *((REPLICATOR_RECEIVE_DATA*)param);
    HANDLE* FinishSignal = replicatorReceiveData.FinishSignal;
    SOCKET* replicatorSocket = replicatorReceiveData.replicatorSocket;
    bool* replicatorConnected = replicatorReceiveData.replicatorConnected;
    bool* registrationSuccessful = replicatorReceiveData.registrationSuccessful;
    char processId[MAX_PROCESS_ID_LENGTH];
    strcpy_s(processId, replicatorReceiveData.processId);

    int iResult;
    char recvBuf[sizeof(MESSAGE)];
    while (WaitForSingleObject(*FinishSignal, 0) != WAIT_OBJECT_0) {
        // Check if connection to replicator broken.
        if (!*replicatorConnected && !*registrationSuccessful) {
            continue;
        }

        if (IsSocketBroken(*replicatorSocket)) {
            shutdown(*replicatorSocket, SD_BOTH);
            closesocket(*replicatorSocket);
            continue;
        }

        if (!IsSocketReadyForReading(replicatorSocket, replicatorConnected)) {
            Sleep(1000);
            continue;
        }

        memset(recvBuf, 0, sizeof(recvBuf));
        iResult = recv(*replicatorSocket, recvBuf, sizeof(MESSAGE), 0);
        if (iResult == 0) {
            printf("Connection with Replicator closed.\n");
            shutdown(*replicatorSocket, SD_BOTH);
            closesocket(*replicatorSocket);
        }
        else if (iResult == SOCKET_ERROR) { // recv failure
            shutdown(*replicatorSocket, SD_BOTH);
            closesocket(*replicatorSocket);
        }
        else { // recv success
            MESSAGE* message = (MESSAGE*)recvBuf;
            printf("\nRECEIVED: ");
            printf("%s\nMESSAGE: ", message->message);
        }
    }

    printf("ReceiveMessageFromReplicator Thread is shutting down.\n");
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
    bool* registrationSuccessful = replicatorData.registrationSuccessful;
    sockaddr_in* serverAddress = replicatorData.serverAddress;
    //char processId[MAX_PROCESS_ID_LENGTH];
    //strcpy_s(processId, replicatorData.processId);
    char* processId = replicatorData.processId;

    int iResult;
    while (WaitForSingleObject(*FinishSignal, 0) != WAIT_OBJECT_0) {
        if (*replicatorConnected) {
            if (!IsSocketBroken(*replicatorSocket)) {
                Sleep(2000);
                continue;
            }

            *replicatorConnected = false;
            *registrationSuccessful = false;
            printf("Connection to the Replicator has been broken.\n");
            shutdown(*replicatorSocket, SD_BOTH);
            closesocket(*replicatorSocket);
        }

        *replicatorSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (*replicatorSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
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

        struct MESSAGE data;
        strcpy_s(data.processId, processId);
        data.flag = REGISTRATION;
        strcpy_s(data.message, "");

        while (!IsSocketReadyForWriting(replicatorSocket)) {
            printf("Cannot send message to replicator.\n");
            Sleep(1000);
        }

        iResult = send(*replicatorSocket, (char*)&data, sizeof(data), 0);
        if (iResult == SOCKET_ERROR) {
            shutdown(*replicatorSocket, SD_BOTH);
            closesocket(*replicatorSocket);
            WSACleanup();
        }

        printf("\nRegistration message sent.\n");

        bool socketReadyToReceive = true;
        while (!IsSocketReadyForReading(replicatorSocket, replicatorConnected))
        {
            if (IsSocketBroken(*replicatorSocket))
            {
                shutdown(*replicatorSocket, SD_BOTH);
                closesocket(*replicatorSocket);
                socketReadyToReceive = false;
                break;
            }
            Sleep(50);
        }

        // Socket was broken and connection was closed.
        if (!socketReadyToReceive) { continue; }

        char recvBuf[DEFAULT_BUFFER_LENGTH];
        iResult = recv(*replicatorSocket, recvBuf, DEFAULT_BUFFER_LENGTH, 0);
        if (iResult == 0) {
            printf("Connection with process closed.\n");
            shutdown(*replicatorSocket, SD_BOTH);
            closesocket(*replicatorSocket);
        }
        else if (iResult == SOCKET_ERROR) { // recv failure
            closesocket(*replicatorSocket);
            WSACleanup();
        }
        else { // recv success
            MESSAGE* message = (MESSAGE*)recvBuf;
            if (message->flag == REGISTRATION_FAILED) {
                // Close socket
                shutdown(*replicatorSocket, SD_BOTH);
                closesocket(*replicatorSocket);

                printf("\nRegistration failed. Connection to Replicator broken.\n");
                *replicatorConnected = false;

                do {
                    printf("Enter New ProcessID: ");
                    gets_s(processId, MAX_PROCESS_ID_LENGTH);
                } while (atoi(processId) <= 0);

            }
            else {
                replicatorData.processId = processId;
                printf("Registration successful.\n\n");
                *registrationSuccessful = true;
            }
        }
    }

    printf("ConnectToReplicator Thread is shutting down.\n");
    return 0;
}