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
Posix or WinAPI compilation) are already included at the head of the
prog.c file you are editing. 

At the end of the examination, the last saved snapshot of this file
will be automatically stored by the system and will be then considered
for the evaluation of your exam. Moifications made to prog.c which you
did not save via the editor will not appear in the stored version
of the prog.c file. 
In other words, unsaved changes will not be tracked, so please save 
this file when you think you have finished software development.
You can also modify the Makefile if requested, since this file will also
be automatically stored together with your program and will be part
of the final data to be evaluated for your exam.

PLEASE BE CAREFUL THAT THE LAST SAVED VERSION OF THE prog.c FILE (and of
the Makfile) WILL BE AUTOMATICALLY STORED WHEN YOU CLOSE YOUR EXAMINATION 
VIA THE CLOSURE CODE YOU RECEIVED, OR WHEN THE TIME YOU HAVE BEEN GRANTED
TO DEVELOP YOUR PROGRAM EXPIRES. 


SPECIFICATION TO BE IMPLEMENTED:
Implementare un programma che riceva in input tramite argv[] i pathname 
associati ad N file, con N maggiore o uguale ad 1. Per ognuno di questi
file generi un thread (quindi in totale saranno generati N nuovi thread 
concorrenti). 
Successivamente il main-thread acquisira' stringhe da standard input in 
un ciclo indefinito, ed ognuno degli N thread figli dovra' scrivere ogni
stringa acquisita dal main-thread nel file ad esso associato.
L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso 
WinAPI) in modo tale che quando uno qualsiasi dei thread dell'applicazione
venga colpito da esso dovra' stampare a terminale tutte le stringhe gia' 
immesse da standard-input e memorizzate nei file destinazione.

*****************************************************************/
#define _GNU_SOURCE
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 512

int num_thread;
char **filenames;
__thread char *printfile;
char buffer[SIZE];
int sem1,sem2;

void *worker(void *param){
    int fd;
    struct sembuf oper;
    int ret;
    long me = (long *)param-1;
    

}

void handler(int sig){
    char buff[128];
    sprintf(buff,"cat %s",printfile);
    system(buff);
}

int main(int argc, char **argv){
    pthread_t tid;
    struct sembuf oper;
    int ret; 

    if(argc<2){
        printf("Not enough arguments. Usage: %s filename1 [filename2] ... [filenameN]\n", argv[0]);
        return -1;
    }

    num_thread=argc-1;
    
    filenames=argv+1;
    printfile=argv[1];

    signal(SIGINT,handler);
    
    sem1=semget(IPC_PRIVATE,1,IPC_CREAT|IPC_EXCL|0666);
    if(sem1==-1){printf("semaphore 1 creation error\n"); return -1;}

    sem2=semget(IPC_PRIVATE,num_thread,IPC_CREAT|IPC_EXCL|0666);
    if(sem2==-1){printf("semaphore 2 creation error\n"); return -1;}

    ret=semctl(sem1,0,SETVAL,num_thread);
    if(ret==-1){printf("error on semaphore init 1\n");return -1;}

    for(int i=0; i<num_thread;i++){
        ret = semctl(sem2,i,SETVAL,0);
        if(ret==-1){printf("error on semaphore init in cycle %d\n",i);return -1;}
    }

    for(long i=0; i<num_thread;i++){
        ret = pthread_create(&tid,NULL,worker, (void *)i+1);
        if(ret ==-1){printf("thread creation error on thread %d",i); return -1;}
    }

    while(1){
        oper.sem_num=0;
        oper.sem_op=-(num_thread);
        oper.sem_flg=0;

redo1:  
        ret=semop(sem1,&oper,1);
        if(ret==-1){
            if(errno==EINTR){
                goto redo1;
            }else{printf("error on semop 1");return -1;}
        }

redo2:
        ret=scanf("%s",buffer);
        if(ret==EOF){
            if(errno==EINTR){goto redo2;}
            else{printf("erroro on while scanf\n");return -1;}
        }
        oper.sem_op=1;
        oper.sem_flg=0;
        for(int i=0;i<num_thread;i++){
            
            oper.sem_num=i;
redo3:
            ret=semop(sem2,&oper,1);
            if(ret==-1){
                if(errno==EINTR){goto redo3;}
                else{
                    printf("error on semop 2\n"); return -1;
                }
            }
        }
    }   



    return 0;
}