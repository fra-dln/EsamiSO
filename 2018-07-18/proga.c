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
for the evaluation of your exam. Modifications made to prog.c which you
did not save via the editor will not appear in the stored version
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
associati ad N file, con N maggiore o uguale ad 1. Per ognuno di questi
file generi un processo che legga tutte le stringhe contenute in quel file
e le scriva in un'area di memoria condivisa con il processo padre. Si 
supponga per semplicita' che lo spazio necessario a memorizzare le stringhe
di ognuno di tali file non ecceda 4KB. 
Il processo padre dovra' attendere che tutti i figli abbiano scritto in 
memoria il file a loro associato, e successivamente dovra' entrare in pausa
indefinita.
D'altro canto, ogni figlio dopo aver scritto il contenuto del file nell'area 
di memoria condivisa con il padre entrera' in pausa indefinita.
L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo padre venga colpito da esso dovra' 
stampare a terminale il contenuto corrente di tutte le aree di memoria 
condivisa anche se queste non sono state completamente popolate dai processi 
figli.

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

#define SIZE 4096

int c;
int fd;
int sem;
struct sembuf oper;
void **mem;
char *segment;
int ret;

void handler(int signo, siginfo_t *a, void *b){
    int i;
    segment; 

    printf("Handler activated, signal caught\n");

    for(i=1; i<c; i++){
        segment = mem[i];
        while(strcmp(segment,"\0")!=0){
            printf("%s\n",segment);
            segment += strlen(segment+1);
        }
    }
}
void child_funct(){
    FILE *f;

    f=fdopen(fd,"r");

    if(f==NULL){printf("fdopen error\n"); exit(-1);}
    
    while(fscanf(f,"%s",segment)!= EOF){
        printf("read %s\n", segment);
        segment += strlen(segment)+1;
    }
    oper.sem_num = 0;
	oper.sem_op = 1;
	oper.sem_flg = 0;

    semop(sem, &oper, 1);

    while(1) pause();
}

int main(int argc, char **argv){
    struct sigaction act;
    sigset_t set;

    c=argc;

    if(c<2){
        printf("Usage: %s filename1 [filename2] ... [filename N]\n");
        return -1;
    }

    for(int i=1; i<c; i++){
        fd=open(argv[i], O_RDONLY);
        if(fd==-1){
            printf("File %s not exist  \n",argv[i]);
            return -1;
        }
    }

    sem=semget(IPC_PRIVATE, 1 ,0660);
    if(sem==-1){
        printf("semget error\n"); return -1;
    }   

    semctl(sem,0,SETVAL,0);

    mem = malloc((c)*sizeof(void *));
    if(mem == NULL){
        printf("malloc error \n"); return -1;
    }

    sigfillset(&set);
    act.sa_sigaction = handler;
    act.sa_mask=set;
    act.sa_flags=0;

    sigaction(SIGINT, &act, NULL);

    for(int i=1; i<c; i++){
        mem[i]=mmap(NULL, SIZE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);
        if(mem[i]==NULL){
            printf("mmap error\n"); return -1;
        }
        segment = mem[i];

        fd = open(argv[i], O_RDONLY);
        ret=fork();
        if(ret==-1){printf("Fork error\n"); return -1;}
        if(ret==0){
            signal(SIGINT,SIG_IGN);
            child_funct();
        }
    }

    oper.sem_num=0; oper.sem_op = -(argc-1); oper.sem_flg = 0;

redo:
    ret=semop(sem, &oper, 1);
    if(ret==-1 && errno == EINTR){goto redo;}
    if(ret==-1 && errno != EINTR){
        printf("error on semop\n"); return -1;
    }

    printf("all childs unlocked the semaphore. \n");

    while(1) pause();

    return 0;
}