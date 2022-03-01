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

//posix version
//please compile with 'gcc prog-B-solution.c -lpthread -DPosix_compile'

#define SIZE 128

char buffer[SIZE];

int sd,sd1;

__thread char* path_name;

char** file_names;

void print(int unused){

	char buffer[SIZE];

	sprintf(buffer,"cat %s", path_name);
	system(buffer);
	return;

}



void * worker(void* a){

	int fd;
	struct sembuf oper;
	int ret;
	long me = (long)a-1;

	path_name = (char*)file_names[me+1];

	fd = open(path_name,O_CREAT|O_TRUNC|O_RDWR,0666);
	if(fd == -1){
		printf("error opening file %s\n",path_name);
		exit(-1);
	}

	printf("file %s correctly opened\n",path_name);

	while(1){

		oper.sem_num = me;
		oper.sem_op = -1;
		oper.sem_flg = 0;
redo1:
		ret = semop(sd1, &oper, 1);
		if(ret == -1){
			if (errno == EINTR) {
				goto redo1;
			}
			else{
				printf("semop error (1)\n");
				exit(1);
			}
		}
		
redo2:
		ret = write(fd,buffer,strlen(buffer)+1);
		if(ret == -1){
			if (errno == EINTR) {
				goto redo1;
			}
			else{
				printf("write error\n");
				exit(1);
			}
		}
	
		oper.sem_num = 0;
		oper.sem_op = 1;
		oper.sem_flg = 0;
redo3:
		ret = semop(sd, &oper, 1);	
		if(ret == -1){
			if (errno == EINTR) {
				goto redo3;
			}
			else{
				printf("semop error (2)\n");
				exit(1);
			}
		}
	
	}
	return NULL;
}

int main(int argc, char** argv){

	int num_threads;
	int i;
	int ret;
	pthread_t tid;
	struct sembuf oper;

	if(argc < 2){
		printf("usage: prog pathname [pathname] ... [pathname]\n");
		return -1;

	}

	file_names = argv;

	num_threads = argc -1;

	path_name = argv[1];//default assignemnt to main thread for I/O upon SIGINT

	signal(SIGINT,print);

	sd = semget(IPC_PRIVATE,1,IPC_CREAT|IPC_EXCL|0666);
	if(sd == -1){
		printf("semaphore creation error\n");
		exit(-1);
	}

	sd1 = semget(IPC_PRIVATE,num_threads,IPC_CREAT|IPC_EXCL|0666);
	if(sd1 == -1){
		printf("semaphore creation error\n");
		exit(-1);
	}

	ret = semctl(sd,0,SETVAL,num_threads);
	if(ret == -1){
		printf("semaphore initialization error (1)\n");
		exit(-1);
	}

	for(i=0;i<num_threads;i++){
		ret = semctl(sd1,i,SETVAL,0);
		if(ret == -1){
			printf("semaphore initialization error (2)\n");
			exit(-1);
		}
	}


	for (i = 0; i < num_threads; i++){
		ret = pthread_create(&tid,NULL,worker,(void*)((long)i+1));
		if(ret == -1){
			printf("thread creation error\n");
			exit(-1);
		}
	}

	while(1){

		oper.sem_num = 0;
		oper.sem_op = -num_threads;
		oper.sem_flg = 0;
redo1:
		ret = semop(sd, &oper, 1);
		if(ret == -1){
			if (errno == EINTR) {
				goto redo1;
			}
			else{
				printf("semop error (4)\n");
				exit(1);
			}
		}
		
redo2:
		ret = scanf("%s",buffer);
		if (ret == EOF){
			if (errno == EINTR) {
				goto redo2;
			}
			else{
				printf("scanf error\n");
				exit(1);
			}
	
		}		

		oper.sem_op = 1;
		oper.sem_flg = 0;
		for(i=0;i<num_threads;i++){
			oper.sem_num = i;
redo3:
			ret = semop(sd1, &oper, 1);
			if(ret == -1){
				if (errno == EINTR) {
					goto redo3;
				}
				else{
					printf("semop error (3)\n");
					exit(1);
				}
			}
		}

	

	}	

	return 0;
}
