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
Implementare una programma che riceva in input, tramite argv[], un insieme di
stringhe S_1 ..... S_n con n maggiore o uguale ad 1. 
Per ogni stringa S_i dovra' essere attivato un thread T_i.
Il main thread dovra' leggere indefinitamente stringhe dallo standard-input.
Ogni stringa letta dovra' essere resa disponibile al thread T_1 che dovra' 
eliminare dalla stringa ogni carattere presente in S_1, sostituendolo con il 
carattere 'spazio'.
Successivamente T_1 rendera' la stringa modificata disponibile a T_2 che dovra' 
eseguire la stessa operazione considerando i caratteri in S_2, e poi la passera' 
a T_3 (che fara' la stessa operazione considerando i caratteri in S_3) e cosi' 
via fino a T_n. 
T_n, una volta completata la sua operazione sulla stringa ricevuta da T_n-1, dovra'
passare la stringa ad un ulteriore thread che chiameremo OUTPUT il quale dovra' 
stampare la stringa ricevuta su un file di output dal nome output.txt.
Si noti che i thread lavorano secondo uno schema pipeline, sono ammesse quindi 
operazioni concorrenti su differenti stringhe lette dal main thread dallo 
standard-input.

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito esso dovra' 
stampare il contenuto corrente del file output.txt su standard-output.

In caso non vi sia immissione di dati sullo standard-input, l'applicazione 
dovra' utilizzare non piu' del 5% della capacita' di lavoro della CPU.

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


FILE *file;
char **strings;
int num_threads;
char **buffers;
pthread_mutex_t *ready;
pthread_mutex_t *done;

void *thread_fun(void *param);

void print(int useless){system("cat output.txt");}

int main (int argc, char **argv){
    int ret;
    pthread_t t;
    char *aux;

    if(argc<2){
        printf("Not enough arguments. Usage: prog String1 ... StringN\n");
        return -1;
    }    

    file=fopen("otuput.txt","w+");
    if(file==NULL){
        printf("error opening output file\n");
        return -1;
    }

    for(int i=0;i<argc; i++){
        strings[i]=argv[i+1];
    }

    num_threads=argc;

    buffers = (char **)malloc(sizeof(char*)*num_threads);
    ready= (pthread_mutex_t *) malloc(num_threads * sizeof(pthread_mutex_t));
    done= (pthread_mutex_t *) malloc(num_threads * sizeof(pthread_mutex_t));

    if(ready==NULL||done==NULL){printf("Malloc error on mutex\n");return -1;}
    if(buffers==NULL){printf("Malloc error on buffers\n"); return -1;}

    for(int i=0; i<num_threads; i++){
        if(pthread_mutex_init(ready+i,NULL)||pthread_mutex_init(done+i, NULL))
        printf("Error initializing mutexs.\n");
        return -1;
    }    
    
    for(int i=0; i<num_threads; i++){
        ret=pthread_create(&t, NULL, thread_fun,(void *)i);
        if(ret!=0){
            printf("Error spawning thread %d\n",i);
            return -1;
        }
    }

    signal(SIGINT,print);

    while(1){
read:
        ret=scanf("%ms",&aux);
        if(ret==EOF && errno == EINTR)
            goto read;
        printf("String readed: %s, pointer at %p\n",aux,aux);
redo1:
        if(pthread_mutex_lock(done)){
			if (errno == EINTR) goto redo1;
			printf("main thread - mutex lock attempt error\n");
			return -1;
		}
        buffers[0]=aux;
redo2: 
        if(pthread_mutex_unlock(ready)){
			if (errno == EINTR) goto redo2;
			printf("main thread - mutex unlock attempt error\n");
			return -1;
		}        
    }

    return 0;
}

void *thread_fun(void *param){
    sigset_t set;
    int me=(int)param;
    int i,j;
    if(me < num_threads -1){
		printf("thread %d started up - in charge of string %s\n",me,strings[me]);
	}
	else{
		printf("thread %d started up - in charge of the output file\n",me);
	}
	fflush(stdout);

    sigfillset(&set);
    sigprocmask(SIG_BLOCK,&set,NULL);

    while(1){
        if(pthread_mutex_lock(ready+me)){
            printf("mutex lock attempt error\n");
            exit(1);
        }

        printf("thread %d - my string: %s\n", me, buffers[me]);
        fprintf(file, "%s\n", buffers[me]);+
        fflush(file);
        }

        if(me == num_threads-1){//I'm the output thread
			printf("writing string %s to output file\n",buffers[me]);
			fprintf(file,"%s\n",buffers[me]);//each string goes to a new line
			fflush(file);

		}
		else{
			//remove characters from the string
			for(i=0; i<strlen(strings[me]);i++){
				for(j=0;j<strlen(buffers[me]);j++){
					if(*(buffers[me]+j) == *(strings[me]+i)) *(buffers[me]+j) = ' ';
				}

			}	

			if(pthread_mutex_lock(done+me+1)){//wait next thread in the pipeline
				printf("thread - mutex lock attempt error\n");
				exit(EXIT_FAILURE);
			}
			buffers[me+1] = buffers[me];
			if(pthread_mutex_unlock(ready+me+1)){
				printf("thread - mutex unlock attempt error\n");
				exit(EXIT_FAILURE);
			}

		}
        if(pthread_mutex_unlock(done+me)){
			printf("mutex unlock attempt error\n");
			exit(EXIT_FAILURE);
		}
}