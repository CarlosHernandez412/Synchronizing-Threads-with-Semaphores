#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>      /* header file for the POSIX API */
#include <string.h>      /* string handling functions */
#include <errno.h>       /* for perror() call */
#include <pthread.h>     /* POSIX threads */ 
#include <sys/ipc.h>     /* SysV IPC header */
#include <sys/sem.h>     /* SysV semaphore header */
#include <sys/syscall.h> /* Make a syscall() to retrieve our TID */



#define BLOCK 8
#define PSEM 0           /* Index */
#define CSEM 1

void *consumer(void * dummy);
void *producer(void * dummy);

int fib(int n) {
 if (n < 2) return n;
 return fib(n-1) + fib(n-2);
}

char buf[9];
int LIMIT = 50;
int count;

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *_buf;
} my_semun;
//int val = 0;

int semid, ret;
struct sembuf GrabPsem[1];
struct sembuf ReleasePsem[1];
struct sembuf GrabCsem[1];
struct sembuf ReleaseCsem[1];

int main(int argc, char *argv[]) {
    pthread_t pthr, cthr;       /* producer and consumer thread */
    int dummy;
	
	if (argc > 1)
	{
    LIMIT = atoi(argv[1]);
    printf("Limit set to %d\n", LIMIT);
	}
	else 
	{
	    printf("Limit set to %d\n", LIMIT);
	}		
    char pathname[128];
    getcwd(pathname, 200);
    strcat(pathname, "/foo");
    key_t ipckey = ftok(pathname, 18);
    int nsem = 2;   /* Two Semaphores */
    semid = semget(ipckey, nsem, 0666 | IPC_CREAT);
    printf ("Created semaphore with ID: %d\n", semid);
    my_semun.val = 1;   /* Value */
    semctl(semid, PSEM, SETVAL, my_semun);
    my_semun.val = 0;
    semctl(semid, CSEM, SETVAL, my_semun);

    int curval; 
    curval = semctl(semid, PSEM, GETVAL, 0);
    printf("PSEM initial val: %d\n", curval);

    curval = semctl(semid, CSEM, GETVAL, 0);
    printf("CSEM initial val: %d\n", curval);

    /* Actions Associated w/PSEM & CSEM */
    GrabPsem[0].sem_num = PSEM;
    GrabPsem[0].sem_flg = SEM_UNDO;
    GrabPsem[0].sem_op = -1;

    GrabCsem[0].sem_num = CSEM;
    GrabCsem[0].sem_flg = SEM_UNDO;
    GrabCsem[0].sem_op = -1;

    ReleasePsem[0].sem_num = PSEM;
    ReleaseCsem[0].sem_num = CSEM;
    ReleasePsem[0].sem_flg = SEM_UNDO;
    ReleaseCsem[0].sem_flg = SEM_UNDO;
    ReleasePsem[0].sem_op = +1;
    ReleaseCsem[0].sem_op = +1;

   
    /* create consumer thread */
    if (pthread_create(&cthr, NULL,  consumer, (void *)&dummy) != 0)
      fprintf(stderr,"Error creating thread\n");

        /* create producer thread */
    if (pthread_create(&pthr, NULL,  producer, (void *)&dummy) != 0)
      fprintf(stderr,"Error creating thread\n");

    /* Calling pthread_join for consumer */
    if ((pthread_join(cthr, (void*)&ret)) < 0)
      perror("pthread_join");

    /* Calling pthread_join for producer */
    if ((pthread_join(pthr, (void*)&ret)) < 0)
      perror("pthread_join");
    
    printf("Deleting semaphores with ID: %d\n", semid);
    ret = semctl(semid, 0, IPC_RMID);

    exit(EXIT_SUCCESS);
}

/* Consumer Child */
void *consumer(void *dummy)
{
    char fileout[BLOCK];
    FILE *outfile;

    /* Open output file stream for writing */
     outfile = fopen("log", "w");
     if(outfile == NULL) {
        perror("fopen: ");
        exit(EXIT_FAILURE);
    }
    
    pid_t tid = syscall(SYS_gettid);      
    fprintf(stdout,"Consumer thread pid: %d tid: %d \n",getpid(), tid);
    fprintf(outfile,"Consumer thread pid: %d tid: %d \n",getpid(), tid);
    for(int i = 0; i < LIMIT; i++) {
       if( feof(outfile)) {
           break;
       }
        printf("Consumer waiting on CSEM...\n");
        ret = semop(semid, GrabCsem, 1);
        fprintf(outfile, "%s", buf);
        printf("Consumer getting %d characters from the buffer\n", count);  
        memset(buf, 0,sizeof(buf));
        count = 0;
        fib(30);
        printf("Producer signaling CSEM...\n");
        ret = semop(semid, ReleasePsem, 1);
    }
    fscanf(outfile, "\n", getpid(), tid);
    fprintf(outfile, "\n");
    
      
    fclose(outfile);
    printf("consumer thread exit code: %d\n",ret);
    pthread_exit(0);
}


/* Producer Child */
void *producer(void *dummy)
{
    char filein[BLOCK];
    FILE *infile;

    infile = fopen("poem", "r");
    if(infile == NULL) {
        perror("fopen: ");
        exit(EXIT_SUCCESS);
    }
    pid_t tid = syscall(SYS_gettid);
    fprintf(stdout,"Producer thread pid: %d tid: %d \n",getpid(), tid);
    
    for(int i = 0; i < LIMIT; i++) {
         printf("Producer waiting on PSEM...\n"); 
         ret = semop(semid, GrabPsem, 1);
         printf("Producer reading from input file.\n");    
         for(int j = 0; j < BLOCK; j++) {
            char a  = fgetc(infile);
            buf[j] = a; 
            if(feof(infile)) {
                break;
            }
             count++;
        }
        printf("Producer put %d characters on buffer.\n", count);
        fib(40);
        printf("Producer signaling PSEM...\n");
        ret = semop(semid, ReleaseCsem, 1);
        }
     printf("End of file reached.\n");
    fclose(infile);
    printf("producer thread exit code: %d\n",ret);  
    pthread_exit(0);
}
