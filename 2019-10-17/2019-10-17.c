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

int num_threads;
char **buffers;
pthread_mutex_t *ready;
pthread_mutex_t *done;
FILE* output_file;
FILE** source_files;
char **files;

void printer(int dummy){
	int i;
	char *p;
	int ret;

	for(i=0;i<num_threads;i++){
		source_files[i] = fopen(files[i],"r");
        	if(source_files[i] == NULL){
               		printf("file %s opening error\n",source_files[i]);
               		exit(EXIT_FAILURE);
        	}
	}	

	output_file = fopen("output_file","w+");
        if(output_file == NULL){
                printf("file %s opening error\n",output_file);
                exit(EXIT_FAILURE);
                
        }

	i = 0;
	while(1){
		ret = fscanf(source_files[i],"%ms",&p);
		if (ret == EOF) break;
		printf("%s\n",p);
		fflush(stdout);
		fprintf(output_file,"%s\n",p);
		fflush(output_file);
		free(p);
		i = (i+1)%num_threads;

	}

}

void * the_thread(void * param){

        sigset_t set;
        long int me = (long int)param;
        int i,j;
	FILE* target_file;

        printf("thread %d started up - in charge of %s\n",me,files[me]);
        fflush(stdout);

        target_file = fopen(files[me],"w+");
        if(target_file == NULL){
                printf("file %s opening error\n",files[me]);
                exit(EXIT_FAILURE);
                
        }

        sigfillset(&set);
        sigprocmask(SIG_BLOCK,&set,NULL);

        while(1){
                if(pthread_mutex_lock(ready+me)){
                        printf("mutex lock attempt error\n");
                        exit(EXIT_FAILURE);
                }

                printf("thread %d - got string %s\n",me,buffers[me]);

                fprintf(target_file,"%s\n",buffers[me]);//each string goes to a new line
                fflush(target_file);

                if(pthread_mutex_unlock(done+me)){
                        printf("mutex unlock attempt error\n");
                        exit(EXIT_FAILURE);
                }
        }

}

int main(int argc, char** argv) {

        int ret;
        long int i;
        pthread_t tid;
        char *p;
        
        if(argc < 2){
                printf("incorrect number of arguments\n");
                exit(EXIT_FAILURE);
        }


        files = argv+1;

        num_threads = argc-1;
        buffers = (char**)malloc(sizeof(char*)*num_threads);
        if(buffers == NULL){
                printf("buffer pointers allocation failure\n");
                exit(EXIT_FAILURE);
        }

        source_files = (FILE**)malloc(sizeof(FILE*)*num_threads);
        if(source_files == NULL){
                printf("file pointers allocation failure\n");
                exit(EXIT_FAILURE);
        }

        ready = malloc(num_threads * sizeof(pthread_mutex_t));  
        done = malloc(num_threads * sizeof(pthread_mutex_t));   

        if(ready == NULL || done == NULL){
                printf("mutex array allocation failure\n");
                exit(EXIT_FAILURE);
        }

        for (i=0; i<num_threads;i++){
                if (pthread_mutex_init(ready+i,NULL) ||pthread_mutex_init(done+i,NULL) || pthread_mutex_lock(ready+i)){
                        printf("mutex array initialization failure\n");
                        exit(EXIT_FAILURE);
                }
        }

        for (i=0; i<num_threads;i++){
                ret = pthread_create(&tid,NULL,the_thread,(void*)i);
                if(ret != 0){
                        printf("thread spawn failure\n");
                        exit(EXIT_FAILURE);
                }

        }

        signal(SIGINT,printer);

	i = 0;
        while(1){
read_again:
                ret = scanf("%ms",&p);
                if (ret == EOF && errno == EINTR) goto read_again;
                printf("read string (area is at address %p): %s\n",p,p);

redo_1:
                if(pthread_mutex_lock(done+i)){
                        if (errno == EINTR) goto redo_1;
                        printf("main thread - mutex lock attempt error\n");
                        exit(EXIT_FAILURE);
                }
                buffers[i] = p;
redo_2:
                if(pthread_mutex_unlock(ready+i)){
                        if (errno == EINTR) goto redo_2;
                       printf("main thread - mutex unlock attempt error\n");
                        exit(EXIT_FAILURE);
                }
		 
		i = (i+1)%num_threads;

        }

        return 0;

}

