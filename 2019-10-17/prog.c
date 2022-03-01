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
Implementare una programma che riceva in input, tramite argv[], il nomi
di N file (con N maggiore o uguale a 1).
Per ogni nome di file F_i ricevuto input dovra' essere attivato un nuovo thread T_i.
Il main thread dovra' leggere indefinitamente stringhe dallo standard-input 
e dovra' rendere ogni stringa letta disponibile ad uno solo degli altri N thread
secondo uno schema circolare.
Ciascun thread T_i a sua volta, per ogni stringa letta dal main thread e resa a lui disponibile, 
dovra' scriverla su una nuova linea del file F_i. 

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito esso dovra' 
riversare su standard-output e su un apposito file chiamato "output-file" il 
contenuto di tutti i file F_i gestiti dall'applicazione 
ricostruendo esattamente la stessa sequenza di stringhe (ciascuna riportata su 
una linea diversa) che era stata immessa tramite lo standard-input.

In caso non vi sia immissione di dati sullo standard-input, l'applicazione dovra' utilizzare 
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



char **filenames; 
char **buffers;
int num_threads;
FILE *output_file; 
FILE **source_files;
pthread_mutex_t *ready;
pthread_mutex_t *done;

void handler(int dummy){
    char ch;
    char *command;
    int i;
    printf("signal handled...\n");

    for(int i=0;i<num_threads;i++){
        source_files[i]=fopen(filenames[i],"r");
        if(source_files[i]==NULL){
            printf("Error on opening source files\n"); 
            exit(EXIT_FAILURE);
        }
    }
 
    output_file=fopen("output-file.txt","w+");
    if(output_file==NULL){
        printf("Error opening output file.\n");
        exit(EXIT_FAILURE);
    }
    fflush(stdin);
    
    for(i=0;i<num_threads;i++){
        while((ch = fgetc(source_files[i])) != EOF){
            fputc(ch,output_file); 
        }
        fputc('\n',output_file);
    }

    system("cat output-file.txt");
    return;
}


void *thread_w(void *param){
    sigset_t set;
    long int me = (long int) param;
    FILE *target;

    printf("thread %d started up - in charge of %s\n", me, filenames[me]);
    fflush(stdout);
    
    target = fopen(filenames[me], "w+");
    if(target == NULL){
        printf("Thread %d - Error opening target file\n", me);
        exit(EXIT_FAILURE);
    }
    sigfillset(&set);
    sigprocmask(SIG_BLOCK,&set,NULL);

    while(1){
        if(pthread_mutex_lock(ready+me)){
            printf("Thread %d - mutex lock error\n",me);
            exit(EXIT_FAILURE);
        }

        printf("thread %d - got string %s\n",me,buffers[me]);

        fprintf(target,"%s\n", buffers[me]);
        fflush(target);

          if(pthread_mutex_unlock(done+me)){
            printf("Thread %d - mutex unlock error\n",me);
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char **argv){
    int ret;
    pthread_t tid;

    if(argc<2){
        printf("Not enough arguments, usage: %s filename1, [filename2], ..., [filenameN]\nExiting...\n",argv[0]); return -1;
    }
    num_threads=argc-1;
 

    filenames=argv+1;

    source_files = (FILE **)malloc(sizeof(FILE *)*num_threads);
    if(source_files==NULL){
        printf("error on source files malloc\n");
        return -1;
    }
    buffers=malloc(sizeof(char*)*num_threads);
    if(buffers==NULL){
        printf("error on buffers malloc\n");
        return -1;
    }
    ready=malloc(sizeof(pthread_mutex_t)*num_threads);
    done=malloc(sizeof(pthread_mutex_t)*num_threads);
    if(ready ==NULL || done == NULL) {printf("malloc error on mutex\n");return -1;}

    for(int i=0; i<num_threads;i++){
        if(pthread_mutex_init(ready+i,NULL)||pthread_mutex_init(done+i,NULL)||pthread_mutex_lock(ready+i)){
            printf("mutex init error\n"); return -1;
        }
    }

    for(long i=0;i<num_threads;i++){
        ret=pthread_create(&tid,NULL,thread_w,(void *)i);
        if(ret!=0){
            printf("error creating thread %d",i); return -1;
        }
    }
    signal(SIGINT,handler);

    long int i=0; char *p;
    while(1){
        
read:   
        ret=scanf("%ms",&p);
        if(ret==EOF && errno ==EINTR) goto read;
       
redo1:  
        if(pthread_mutex_lock(done+i)){
            if(errno == EINTR) goto redo1;
            printf("main thread, mutex lock error\n"); exit(EXIT_FAILURE);
        }
   
        buffers[i]=p;

redo2: 
        if(pthread_mutex_unlock(ready+i)){
            if(errno == EINTR) goto redo1;
            printf("main thread, mutex unlock error\n"); exit(EXIT_FAILURE);
        }

        i=(i+1)%num_threads;
    }
    return 0;
}