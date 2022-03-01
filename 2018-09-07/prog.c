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
given specification. Most of the requested header files (for either 
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
Implementare un'applicazione che riceva in input tramite argv[] il 
nome di un file F ed una stringa indicante un valore numerico N maggiore
o uguale ad 1.
L'applicazione, una volta lanciata dovra' creare il file F ed attivare 
N thread. Inoltre, l'applicazione dovra' anche attivare un processo 
figlio, in cui vengano attivati altri N thread. 
I due processi che risulteranno attivi verranno per comodita' identificati
come A (il padre) e B (il figlio) nella successiva descrizione.

Ciascun thread del processo A leggera' stringhe da standard input. 
Ogni stringa letta dovra' essere comunicata al corrispettivo thread 
del processo B tramite memoria condivisa, e questo la scrivera' su una 
nuova linea del file F. Per semplicita' si assuma che ogni stringa non
ecceda la taglia di 4KB. 

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo A venga colpito esso dovra' 
inviare la stessa segnalazione verso il processo B. Se invece ad essere 
colpito e' il processo B, questo dovra' riversare su standard output il 
contenuto corrente del file F.

Qalora non vi sia immissione di input, l'applicazione dovra' utilizzare 
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

#define SIZE 4096

int fd;
FILE *file;
char *filename;
char **memory_segs;
int num_threads;
pid_t pid;
int sem1,sem2;
pthread_t tid;

void *child_w(void *); void *parent_w(void *);

void child_handler(int useless){
    char buff[1024];
    sprintf(buff,"cat %s\n",filename);
    system(buff);
} 

void parent_handler(int sig){
    printf("Parent received signal. Forwarding to child.\n");
    printf("%d\n",pid);
    kill(pid,sig);
}


int main(int argc, char **argv){

    if(argc<3){
        printf("Usage: %s filename N\n", argv[0]);
        return -1;
    }

    num_threads=atoi(argv[2]);

    if(num_threads<1){
        printf("Usage: %s filename N. N must be >1\n", argv[0]);
        return -1;
    }

    filename = argv[1];
    file=fopen(filename,"w+");

    if(file == NULL){
        printf("Error opening file %s", filename);
    }
    
    memory_segs= malloc(sizeof(char *)*num_threads);

    if(memory_segs==NULL){
        printf("Malloc error, exiting.\n");return -1;
    }

    for(int i=0; i<num_threads; i++){
        memory_segs[i]=(char *)mmap(NULL,SIZE,PROT_READ|PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS, 0,0);
        if(memory_segs[i]== (void *)-1){
            printf("mmap error on memory segment %d",i);
            return -1;
        }
    }

    sem1=semget(IPC_PRIVATE, num_threads, IPC_CREAT|0666);
    sem2=semget(IPC_PRIVATE, num_threads, IPC_CREAT|0666);
    if(sem1==-1||sem2==-1){
        printf("error on semget\n"); return -1;
    }   

    int ret1,ret2;
    for(int i=0; i<num_threads;i++){
        ret1=semctl(sem1,i,SETVAL,1);
        ret2=semctl(sem2,i,SETVAL,0);
        if(ret1==-1||ret2==-1){
            printf("error on semctl\n"); return -1;
        }
    }

    pid=fork();
    if(pid==-1){printf("Error on fork \n");return -1;}

    if(pid==0){
        signal(SIGINT,child_handler);
        printf("child process with pid:%d\n",(int)getpid());
        for(long i=0; i<num_threads; i++){
            if(pthread_create(&tid,NULL,child_w, (void *)i)==-1){
                printf("child threads creation failed on cicle number %d",i);
                return -1;
            }
        }
        
    }else{
        signal(SIGINT, parent_handler);

        for(long i=0; i<num_threads; i++){
            if(pthread_create(&tid,NULL,parent_w, (void *)i)==-1){
                printf("parent threads creation failed on cicle number %d",i);
                return -1;
            }
        }
     
    }

    while(1) pause();
    
    return 0;
}

void * parent_w(void * arg){
    long me = (long)arg;
    struct sembuf oper;
    int ret;

    oper.sem_num=me;
    oper.sem_flg=0;

    while(1){
        oper.sem_op=-1;
semop1: 
        ret=semop(sem1,&oper,1);
        if(ret==-1 && errno != EINTR){
            printf("semop error (1)\n"); exit(-1);
        }
        if(ret==-1) goto semop1;

scan:
        ret= scanf("%s", memory_segs[me]);
        if (ret == EOF && errno != EINTR){
			printf("scanf error (1)\n");
			exit(-1);
		}
		if (ret == -1) goto scan;

        oper.sem_op=1;

semop2: 
         ret=semop(sem2,&oper,1);
        if(ret==-1 && errno != EINTR){
            printf("semop error (2)\n"); exit(-1);
        }
        if(ret==-1) goto semop2;
    }
    return NULL;
}

void * child_w(void * arg){
    long me = (long)arg;
    struct sembuf oper;
    int ret;

    oper.sem_num=me;
    oper.sem_flg = 0;

    while(1){
semop1:
        oper.sem_op=-1;
        ret = semop(sem2,&oper,1);
		if (ret == -1 && errno != EINTR){
			printf("semop error (3)\n");
			exit(-1);
		}
		if (ret == -1) goto semop1;

        fprintf(file, "%s\n", memory_segs[me]);
        fflush(file);

semop2: 
        oper.sem_op=1;
        ret=semop(sem1, &oper, 1);
        if (ret == -1 && errno != EINTR){
            printf("%d",ret);
			printf("semop error (4)\n");
			exit(-1);
		}
		if (ret == -1) goto semop2;
    }
    return NULL;

}