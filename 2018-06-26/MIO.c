#define _GNU_SOURCE
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _data{
    int val;
    struct _data *next;
} data;

pthread_mutex_t lock,next;

data *lists;
int num_threads;
int val;

void printer(int signo){
    data aux; int i;

    for(i=0;i<num_threads;i++){
        aux=lists[i];
        printf("printing list %d...\n\n", i);
        while(aux.next){
            printf("%d", aux.next->val);
            aux= *(aux.next);
        }
        printf("\n");
    }
}
void *worker(void *dumb){
    data *aux; int ret;

    printf("thread %ld alive\n", (long)dumb);
    while(1){
        aux = (data*)malloc(sizeof(aux));
        if(aux==NULL){printf("AUX malloc error\n");exit(-1);}
        ret = pthread_mutex_lock(&lock);
        if(ret!=0){printf("locking error on worker\n"); exit(-1);}

        printf("thread %ld - found value is %d\n",(long)dumb,val);
		aux->val = val;

        ret = pthread_mutex_unlock(&next);
		if (ret != 0){
			printf("unlocking error\n");
			exit(-1);
		}

        aux->next = lists[(long)dumb].next;
		lists[(long)dumb].next = aux;

    }
    return 0;
}

int main(int argc, char **argv){

    long i;
    pthread_t tid;
    int ret;

    sigset_t set;
    struct sigaction act;

    if(argc<2){
        printf("Error, not enough arguments were passed by. Usage: prog num_thread\n");
        return -1;
    }

    num_threads=strtol(argv[1],NULL,10);
    printf("Spawning %d threads. If you put too many just run away, your PC is gonna explode...\n", num_threads);

    ret=pthread_mutex_init(&lock, NULL);
    if(ret!=0){
        printf("lock mutex init error"); return -1;
    }

    ret=pthread_mutex_init(&lock, NULL);
    if(ret!=0){
        printf("next mutex init error"); return -1;
    }

    ret = pthread_mutex_lock(&lock);
	if (ret != 0){
		printf("locking error\n");
		exit(-1);
	}

    signal(SIGINT,printer);

    for(i=0;i<num_threads;i++){
        ret=pthread_create(&tid,NULL,worker, (void *)i);
        if(ret!=0){
            printf("Error creating thread %d\n", i);
            return -1;
        }
    }
    
    while(1){
step1:
        ret=pthread_mutex_lock(&next);
        if(ret!=0&&errno==EINTR) goto step1;

step2: 
        ret=scanf("%d",&val);
        if(ret==EOF&&errno == EINTR) goto step2;
        if(ret==0) {printf("non compliant input, only integer accepted \n"); return -1;}

step3:  
        ret=pthread_mutex_unlock(&lock);
        if(ret!=0 && errno == EINTR ) goto step3;
    }
    
    return 0;
}