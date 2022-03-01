/******************************************************************
Welcome to the Operating System examination

You are editing the '/home/esame/prog.c' file. You cannot remove 
this file, just edit it so as to produce your own program according to
the specification listed below.

In the '/home/esame/'directory you can find a Makefile that you can 
use to compile this prpogram to generate an executable named 'prog' 
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
Implementare una programma che riceva in input, tramite argv[], il nome
di un file F ed N stringhe S_1 .. S_N (con N maggiore o uguale
ad 1.
Per ogni stringa S_i dovra' essere attivato un nuovo thread T_i, che fungera'
da gestore della stringa S_i.
Il main thread dovra' leggere indefinitamente stringhe dallo standard-input. 
Ogni nuova stringa letta dovra' essere comunicata a tutti i thread T_1 .. T_N
tramite un buffer condiviso, e ciascun thread T_i dovra' verificare se tale
stringa sia uguale alla stringa S_i da lui gestita. In caso positivo, ogni
carattere della stringa immessa dovra' essere sostituito dal carattere '*'.
Dopo che i thread T_1 .. T_N hanno analizzato la stringa, ed eventualmente questa
sia stata modificata, il main thread dovra' scrivere tale stringa (modificata o non)
su una nuova linea del file F. 
In altre parole, la sequenza di stringhe provenienti dallo standard-input dovra' 
essere riportata su file F in una forma 'epurata'  delle stringhe S1 .. SN, 
che verranno sostituite da strighe  della stessa lunghezza costituite esclusivamente
da sequenze del carattere '*'.
Inoltre, qualora gia' esistente, il file F dovra' essere troncato (o rigenerato) 
all'atto del lancio dell'applicazione.

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito esso dovra' 
riversare su standard-output il contenuto corrente del file F.

Qualora non vi sia immissione di input, l'applicazione dovra' utilizzare 
non piu' del 5% della capacita' di lavoro della CPU.

*****************************************************************/
#define _GNU_SOURCE

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
#include <string.h>

typedef struct __thread_args{
    int id;
    char *string;
}thread_args;

int num_thread;
char *filename;
int fd;

sem_t *write_sem;
sem_t *read_sem;

char buff[1024];

void *thread_fun(void *arg);

void printer(){
    char cmd[4096];
    printf("\n");
    sprintf(cmd,"cat %s", filename);
    system(cmd);
}

int main(int argc, char **argv){

    pthread_t thread;
    struct sigaction sa;
    int ret;


    if(argc < 3){
        printf("Not enough arguments. Usage: prog filename.txt string 1 ... string n \n");
        return -1;
    }   

    num_thread = argc -2;
    filename = argv[1];

    fd = open(filename, O_CREAT|O_RDWR|O_TRUNC, 0660);
    if(fd==-1){
        printf("Error opening the file. Exiting..."); return -1;
    }

	write_sem = (sem_t *)(malloc(num_thread*sizeof(sem_t)));
    read_sem = (sem_t *)(malloc(num_thread*sizeof(sem_t)));

    if(write_sem==NULL||read_sem==NULL){
        printf("Malloc error\n");
        return -1;
    }

    for (int i= 0; i<num_thread; i++) {
		sem_init(&write_sem[i], 0, 0);
		sem_init(&read_sem[i], 0, 1);
	}

    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = (__sighandler_t)printer;

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		printf("sigaction error\n");
		exit(-1);	
	}

    for (int i = 0; i<num_thread; i++){
        thread_args *curr = (thread_args *)malloc(sizeof(thread_args));
        curr->id=i; curr->string=argv[i+2];
        int r = pthread_create(&thread, NULL, thread_fun, (void *)curr);
        if(r!= 0 ){
            printf("pthread_create error\n"); return -1;
        }
    }

    strcpy(buff, "\0");

    FILE *fp = fdopen(fd, "r+");
	if (fp == NULL) {
		printf ("fdopen error\n");
		exit(EXIT_FAILURE);	
	}
    
    while(1){
        for(int i=0; i<num_thread; i++){
step1:     
            ret=sem_wait(&read_sem[i]);
            if(ret!=0 && errno == EINTR){goto step1;}            
        }

        if(strcmp(buff,"\0")!= 0){
            fprintf(fp, "%s\n", buff);
            fflush(fp);
        }

        while(scanf("%s",buff)<=0);
        for (int i=0; i<num_thread; i++){
step2:      
            ret=-1;
            ret=sem_post(&write_sem[i]);
            if (ret != 0 && errno == EINTR) { goto step2; }             
        }

    }

    return 0;
}

void *thread_fun(void *arg){
    int ret;
    thread_args *mine;
    mine = (thread_args*)arg;
    int me = mine->id;

    printf("Hey it's thread %d with string %s\n",me, mine->string);

    while(1){
step1:
        ret=sem_wait(&write_sem[me]);
        if(ret!=0 && errno == EINTR){goto step1;}

        if(strcmp(buff, mine->string)==0){
            strcpy(buff, "*");
            for(int i = 0; i<(strlen(mine->string)-1); i++){
                strcat(buff,"*");
            }
        }
step2:
        ret = sem_post(&read_sem[me]);
		if (ret != 0 && errno == EINTR) { goto step2; }
    }
}
