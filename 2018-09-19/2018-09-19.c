/******************************************************************
Welcome to the Operating System examination

You are editing the '/home/esame/prog.c' file. You cannot remove
this file, just edit it so as to produce your own program according to
the specification listed below.

In the '/home/esame/'directory you can find a Makefile that you can
use to compile this program to generate an executable named 'prog'
in the same directory. Typing 'make posix' you will compile for
Posix, while typing 'make winapi' you will compile for WinAPI just
depending on the specific technology you selected to implement the
given specification. Most of the required header files (for either
Posix or WinAPI compilation) are already included in the head of the
prog.c file you are editing.

At the end of the examination, the last saved snapshot of this file
will be automatically stored by the system and will be then considered
for the evaluation of your exam. Moifications made to prog.c which are
not saved by you via the editor will not appear in the stored version
of the prog.c file.
In other words, unsaved changes will not be tracked, so please save
this file when you think you have finished software development.
You can also modify the Makefile if requesed, since this file will also
be automatically stored together with your program and will be part
of the final data to be evaluated for your exam.

PLEASE BE CAREFUL THAT THE LAST SAVED VERSION OF THE prog.c FILE (and of
the Makfile) WILL BE AUTOMATICALLY STORED WHEN YOU CLOSE YOUR EXAMINATION
VIA THE CLOSURE CODE YOU RECEIVED, OR WHEN THE TIME YOU HAVE BEEN GRANTED
TO DEVELOP YOUR PROGRAM EXPIRES.


SPECIFICATION TO BE IMPLEMENTED:
Implementare un programma che riceva in input tramite argv[] i pathname
associati ad N file (F1 ... FN), con N maggiore o uguale ad 1.
Per ognuno di questi file generi un thread che gestira' il contenuto del file.
Dopo aver creato gli N file ed i rispettivi N thread, il main thread dovra'
leggere indefinitamente la sequenza di byte provenienti dallo standard-input.
Ogni 5 nuovi byte letti, questi dovranno essere scritti da uno degli N thread
nel rispettivo file. La consegna dei 5 byte da parte del main thread
dovra' avvenire secondo uno schema round-robin, per cui i primi 5 byte
dovranno essere consegnati al thread in carico di gestire F1, i secondi 5
byte al thread in carico di gestire il F2 e cosi' via secondo uno schema
circolare.

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito esso dovra',
a partire dai dati correntemente memorizzati nei file F1 ... FN, ripresentare
sullo standard-output la medesima sequenza di byte di input originariamente
letta dal main thread dallo standard-input.

Qualora non vi sia immissione di input, l'applicazione dovra' utilizzare
non piu' del 5% della capacita' di lavoro della CPU.

*****************************************************************/
#ifdef Posix_compile
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <fcntl.h>
#else
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//THIS SOLUTION IS FOR WINAPI - please keep the below 2 includes if you are compiling with Visual Studio
#include "stdafx.h"
#include <windows.h>

#define NUM_CHARS (5)
char the_buffer[NUM_CHARS];
char other_buffer[NUM_CHARS];

LPHANDLE handles;
LPHANDLE semaphores;
LPHANDLE reconstruction_handles;

int num_threads;

int  theHandler(int event){
	DWORD res;
	int turn;
	int i;

	printf("received CTRL+C - reconstructing the original stream\n");
	fflush(stdout);

	for (i = 1; i <= num_threads; i++){
		if (SetFilePointer(reconstruction_handles[i], 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER){
			printf("Cannot position at the begin of file\n");
			fflush(stdout);
			ExitProcess(-1);
		}
	}

	turn = 0;

	while (1) {
			res = 0;
			if (ReadFile(reconstruction_handles[turn + 1], other_buffer, NUM_CHARS, &res, NULL) == FALSE){
				printf("Cannot read from file\n");
				fflush(stdout);
				ExitProcess(-1);
			}
		
		for (i = 0; i < res; i++){
			putchar(other_buffer[i]);
		}
		fflush(stdout);

		if (res < NUM_CHARS){
			printf("\n stream reconstruction done\n");
			fflush(stdout);
			return 1;
		}

		turn = (turn + 1) % (num_threads);

	}
}

void theThread(char *param){
	int me = (int)param;
	DWORD res;

	while(1){
		WaitForSingleObject(semaphores[me], INFINITE);
		printf("thread %d: getting %d more bytes to write on file\n", me, NUM_CHARS);
		fflush(stdout);
		if (WriteFile(handles[me], the_buffer, NUM_CHARS, &res, NULL) == FALSE){
			printf("Cannot write to destination file\n");
			ExitProcess(-1);
		}
		ReleaseSemaphore(semaphores[0], 1, NULL);
		printf("thread %d: %d more bytes written to file\n", me, NUM_CHARS);
		fflush(stdout);

	}
	return;
}

int main(int argc, char** argv){

	int i;
	int turn;

	if (argc < 2){
		printf("usage: prog file_name1 [file_name2 ... file_nameN]\n");
		return - 1;
	}
	
	num_threads = argc - 1;

	handles = (LPHANDLE)malloc((argc)*sizeof(LPHANDLE));
	reconstruction_handles = (LPHANDLE)malloc((argc)*sizeof(LPHANDLE));

	if (handles == NULL || reconstruction_handles == NULL){
		printf("Cannot allocate handles arrays\n");
		return -1;
	}

	handles[0] = NULL;

	for (i = 1; i < argc; i++){
		handles[i] = CreateFile(argv[i], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		reconstruction_handles[i] = CreateFile(argv[i], GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (handles[i] == INVALID_HANDLE_VALUE || reconstruction_handles[i] == INVALID_HANDLE_VALUE) {
			printf("Cannot create destination file %s\n", argv[i]);
			return -1;
		}
	}


	semaphores = (LPHANDLE)malloc((argc)*sizeof(LPHANDLE));
	if (handles == NULL){
		printf("Cannot allocate semphores array\n");
		return -1;
	}

	semaphores[0] = CreateSemaphore(NULL, 1, 1, NULL);
	if (semaphores[0] == INVALID_HANDLE_VALUE){
		printf("Cannot create semaphore[0]\n");
		return -1;
	}

	for (i = 1; i < argc; i++){
		semaphores[i] = CreateSemaphore(NULL, 0, 1, NULL);
		if (semaphores[i] == INVALID_HANDLE_VALUE){
			printf("Cannot create semaphore[%d]\n",i);
			return -1;
		}

	}

	for (i = 1; i < argc; i++){
		if (CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)theThread, (LPVOID)i, NORMAL_PRIORITY_CLASS, NULL) == INVALID_HANDLE_VALUE){
			printf("Cannot create thread %d\n", i);
			return -1;
		} 
	}

	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)theHandler, 1) == FALSE){
		printf("Cannot set  CTRL+C handler\n", i);
		return -1;
	}

	turn = 0;
	while (1){
		WaitForSingleObject(semaphores[0], INFINITE);
		for (i = 0; i < 5; i++) the_buffer[i] = getchar();
		ReleaseSemaphore(semaphores[turn+1], 1, NULL);
		turn = (turn + 1) % (argc - 1);
	}
	return 0;
}


