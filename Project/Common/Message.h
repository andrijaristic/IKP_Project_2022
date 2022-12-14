#pragma once

#define MAX_BUFFER_LENGTH 512

// dopuni po potrebi
typedef enum FLAGS {
	MESSAGE,
	REGISTRATION,
	REGISTRATION_SUCCESSFUL,
	REGISTRATION_FAILED,
	INTEGRITY_UPDATE
}FLAGS;

typedef struct MESSAGE
{
	short processId;
	char message[MAX_BUFFER_LENGTH];
	FLAGS flag;
}MESSAGE;