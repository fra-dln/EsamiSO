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
#include <sys/stat.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUM_CHAR 5

char *S;
int ready,done;
char *buff;
char *filename_diretto;
char *filename_inverso;
int fd[2];

void *worker1(void *param); void handler(void *dummy);

int main(int argc, char **argv){
    pthread_t tid;
    int ret; 
    struct sembuf op;
    if(argc<2){
        pritnf("not enough arguments. Usage: %s String\n",argv[0]);
        return -1;
    }

    S=argv[1];

    filename_diretto=(char *)malloc(sizeof(char *));
    filename_inverso=(char *)malloc(sizeof(char *));
    if(filename_diretto==NULL||filename_inverso==NULL){
        printf("malloc error\n"); return -1;
    }

    sprintf(filename_diretto,"%s_diretto",S);
    sprintf(filename_inverso,"%s_inverso",S);

    ready=semget(IPC_PRIVATE,2,IPC_CREAT|0666);
    done=semget(IPC_PRIVATE,2,IPC_CREAT|0666);

    if(ready == -1|| done ==-1){
        printf("error creating semaphores\n"); return -1;
    }

    semctl(ready,0,SETVAL,1);
    semctl(ready,1,SETVAL,1);
    semctl(done,0,SETVAL,0);
    semctl(done,1,SETVAL,0);
    
    for(long i=0;i<2;i++){
        ret = pthread_create(&tid,NULL,worker1,(void *)(i+1));
        if(ret == -1){
            printf("error spawning T1");
            return -1;
        }
    }

    signal(SIGINT,handler);

    while(1){   
        for(int i=0; i<2; i++){
redo:   
            op.sem_flg=0;
            op.sem_num=i;
            op.sem_op=-1; 

            ret = semop(ready,&op,1);
            if(ret==-1){
                if(errno==EINTR){
                    goto redo;
                }else{
                    printf("erroro on semop\n"); return -1;
                }
            }
        }

        for(int j=0; j< NUM_CHAR;j++)
            ret = scanf("%c",buff+j);   
            if(ret==-1){
                printf("scanf error\n"); return -1;
            }            
        
        for(int i=0; i<2; i++){
redo1:  
        op.sem_num=i;
        op.sem_flg=0;
        op.sem_op=1;

        ret=semop(done,&op,1);
        if(ret==-1){
                if(errno==EINTR){
                    goto redo;
                }else{
                    printf("erroro on semop\n"); return -1;
                }
            }
        }

    }
    return 0;
}

void *worker1(void *param){
    long me = (long *)param;
    if(me == 1){
        fd[0]=open(filename_diretto,O_CREAT|O_TRUNC|O_RDWR,S_IRWXU|S_IRWXG|S_IRWXO);



    }else{



    }


}