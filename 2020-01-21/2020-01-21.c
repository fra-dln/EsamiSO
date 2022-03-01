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
for the evaluation of your exam. Modifications made to prog.c which are
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
Implementare una programma che ricevento in input tramite argv[] una stringa S
esegua le seguenti attivita'.
Il main thread dovra' attivare due nuovi thread, che indichiamo con T1 e T2.
Successivamente il main thread dovra' leggere indefinitamente caratteri dallo 
standard input, a blocchi di 5 per volta, e dovra' rendere disponibili i byte 
letti a T1 e T2. 
Il thread T1 dovra' inserire di volta in volta i byte ricevuti dal main thread 
in coda ad un file di nome S_diretto, che dovra' essere creato. 
Il thread T2 dovra' inserirli invece nel file S_inverso, che dovra' anche esso 
essere creato, scrivendoli ogni volta come byte iniziali del file (ovvero in testa al 
file secondo uno schema a pila).

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito esso dovra' 
calcolare il numero dei byte che nei due file hanno la stessa posizione ma sono
tra loro diversi in termini di valore. Questa attivita' dovra' essere svolta attivando 
per ogni ricezione di segnale un apposito thread.

In caso non vi sia immissione di dati sullo standard input, l'applicazione dovra' 
utilizzare non piu' del 5% della capacita' di lavoro della CPU.

*****************************************************************/

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


#include <stdio.h>
#include <stdlib.h>

#define SIZE (4096)

#define NUM_THREADS (2)

HANDLE hThread[NUM_THREADS];


char buffer[SIZE];

char *string;


HANDLE sem_ready[NUM_THREADS];

HANDLE sem_done[NUM_THREADS];

#define NUM_CHARS (5)


int written_bytes = -NUM_CHARS;


int the_handler(int event) {

        char* aux;
        HANDLE handles[2];
        int i;
        char c0, c1;
        DWORD ret;
        int count = 0;



        printf("received CTRL+C\n");
        fflush(stdout);


        aux = (char*)malloc(strlen(string) + 128);
        if (aux == NULL) {
                printf("handler - unable to allocate area for file name\n");
                fflush(stdout);
                ExitProcess(1);

        }

        for (i = 0; i < 2; i++) {

                sprintf(aux, "%s%s", string, i == 0 ? "_diretto" : "_inverso");

                handles[i] = CreateFile(aux, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (handles[i] == INVALID_HANDLE_VALUE) {
                        printf("unable to open file %s\n", aux);
                        fflush(stdout);
                        ExitProcess(1);
                }

        }


        for (i = 0; i < written_bytes; i++) {
                ReadFile(handles[0], &c0, 1, &ret, NULL);
                ReadFile(handles[1], &c1, 1, &ret, NULL);
                if (c0 != c1) count++;
        }

        CloseHandle(handles[0]);
        CloseHandle(handles[1]);
        free(aux);

        printf("num of different chars is %d\n",count);
        fflush(stdout);

        return  1;

}




WORD the_thread(void* param) {

        long me = (int)param;
        HANDLE the_file;
        char * aux;
        DWORD ret;
        char temp[NUM_CHARS];
        int current_size;

        printf("thread %d ready - in charge of file %s%s\n",me,string,me==0?"_diretto":"_inverso");
        fflush(stdout);

        aux = (char*)malloc(strlen(string) + 128);
        if (aux == NULL) {
                printf("unable to allocate area for file name\n");
                fflush(stdout);
                ExitProcess(1);

        }

        sprintf(aux, "%s%s", string, me == 0 ? "_diretto" : "_inverso");

        the_file = CreateFile(aux, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (the_file == INVALID_HANDLE_VALUE) {
                printf("unable to create file %s\n",aux);
                fflush(stdout);
                ExitProcess(1);
        }


retry:
        WaitForSingleObject(sem_ready[me], INFINITE);

        WriteFile(the_file, buffer, NUM_CHARS, &ret, NULL);


        if (me == 1) {
                current_size = SetFilePointer(the_file, 0, NULL, FILE_CURRENT);

                SetFilePointer(the_file, 0, NULL, FILE_BEGIN);
                while (current_size > 0) {
                        ReadFile(the_file, temp, NUM_CHARS, &ret, NULL);
                        SetFilePointer(the_file, -NUM_CHARS, NULL, FILE_CURRENT);
                        WriteFile(the_file, buffer, NUM_CHARS, &ret, NULL);
                        memcpy(buffer, temp, NUM_CHARS);
                        current_size -= NUM_CHARS;
                }

        }


        ReleaseSemaphore(sem_done[me], 1, NULL);
        goto retry;

}



int main(int argc, char *argv[]) {


        DWORD hid;
        int i;
        int ret;
        BOOL result;

        if (argc != 2) {
                printf("wrong numberof arguments\n");
                fflush(stdout);
                ExitProcess(1);

        }

        for (i = 0; i < NUM_THREADS; i++) {
                sprintf(buffer, "ready%d", i);
                sem_ready[i] = CreateSemaphore(NULL, 0, 1, buffer);
                sprintf(buffer, "done%d", i);
                sem_done[i] = CreateSemaphore(NULL, 1, 1, buffer);
                if (sem_ready[i] == INVALID_HANDLE_VALUE || sem_done[i] == INVALID_HANDLE_VALUE) {
                        printf("semaphores creation error");
                        fflush(stdout);
                        ExitProcess(1);
                }

        }

        result = SetConsoleCtrlHandler((PHANDLER_ROUTINE)the_handler, 1);
        if (result == FALSE) {
                printf("handler posting error");
                fflush(stdout);
                ExitProcess(1);
        }

        string = argv[1];

        for (i = 0; i < NUM_THREADS; i++) {

                hThread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)the_thread, (void*)i, NORMAL_PRIORITY_CLASS, &hid);

        }


        while (1) {



                WaitForMultipleObjects(NUM_THREADS, sem_done, TRUE, INFINITE);

                written_bytes += NUM_CHARS;

                for (i = 0; i < NUM_CHARS; i++) {
                        ret = scanf("%c", buffer+i);
                        if (ret == EOF) {
                                i--;
                        }
                }

                for (i = 0; i < NUM_THREADS; i++) {
                        ReleaseSemaphore(sem_ready[i], 1, NULL);
                }


        }

        return 0;

}
