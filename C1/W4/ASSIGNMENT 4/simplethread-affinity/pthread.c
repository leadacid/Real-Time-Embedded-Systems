#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sched.h>
#include <unistd.h> // This include was missing from sample code, giving a warning
#include <syslog.h>
#include <sys/utsname.h>
#include <unistd.h>
#define NUM_THREADS 128
#define NUM_CPUS 8

typedef struct
{
    int threadIdx;
} threadParams_t;


// Structure require by utsname to be able to use uname
struct utsname unameData;

// POSIX thread declarations and scheduling attributes
//
pthread_t threads[NUM_THREADS];
pthread_t mainthread;
pthread_t startthread;
threadParams_t threadParams[NUM_THREADS];

pthread_attr_t fifo_sched_attr;
pthread_attr_t orig_sched_attr;
struct sched_param fifo_param;

#define SCHED_POLICY SCHED_FIFO
#define MAX_ITERATIONS (1000000)


void print_scheduler(void)
{
    int schedType = sched_getscheduler(getpid());

    switch(schedType)
    {
        case SCHED_FIFO:
            syslog(LOG_INFO, "Pthread policy is SCHED_FIFO\n");
            break;
        case SCHED_OTHER:
            syslog(LOG_INFO, "Pthread policy is SCHED_OTHER\n");
            break;
        case SCHED_RR:
            syslog(LOG_INFO, "Pthread policy is SCHED_RR\n");
            break;
        default:
            syslog(LOG_INFO, "Pthread policy is UNKNOWN\n");
    }
}


void set_scheduler(void)
{
    int max_prio, rc, cpuidx;
    cpu_set_t cpuset;

    syslog(LOG_INFO, "INITIAL ");
    print_scheduler();

    pthread_attr_init(&fifo_sched_attr);
    pthread_attr_setinheritsched(&fifo_sched_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&fifo_sched_attr, SCHED_POLICY);
    CPU_ZERO(&cpuset);
    cpuidx=(3);
    CPU_SET(cpuidx, &cpuset);
    pthread_attr_setaffinity_np(&fifo_sched_attr, sizeof(cpu_set_t), &cpuset);

    max_prio=sched_get_priority_max(SCHED_POLICY);
    fifo_param.sched_priority=max_prio;    

    if((rc=sched_setscheduler(getpid(), SCHED_POLICY, &fifo_param)) < 0)
        perror("sched_setscheduler");

    pthread_attr_setschedparam(&fifo_sched_attr, &fifo_param);

    syslog(LOG_INFO, "ADJUSTED ");
    print_scheduler();
}




void *counterThread(void *threadp)
{
    int sum=0, i, iterations;
    threadParams_t *threadParams = (threadParams_t *)threadp;
    double start=0.0, stop=0.0;
    struct timeval startTime, stopTime;

    gettimeofday(&startTime, 0);
    start = ((startTime.tv_sec * 1000000.0) + startTime.tv_usec)/1000000.0;


    for(iterations=0; iterations < MAX_ITERATIONS; iterations++)
    {
        sum=0;
        for(i=1; i < (threadParams->threadIdx)+1; i++)
            sum=sum+i;
    }


    gettimeofday(&stopTime, 0);
    stop = ((stopTime.tv_sec * 1000000.0) + stopTime.tv_usec)/1000000.0;

    printf("Start:%f Stop %f\n", start, stop);	    
    syslog(LOG_INFO, "[COURSE:1][ASSIGNMENT:4]: Thread idx=%d, sum[0...%d]=%d Running on core : %d\n", 
           threadParams->threadIdx,
           threadParams->threadIdx, sum, sched_getcpu());
    printf("[COURSE:1][ASSIGNMENT:4]: Thread idx=%d, sum[0...%d]=%d Running on core : %d\n", 
           threadParams->threadIdx,
           threadParams->threadIdx, sum, sched_getcpu());
    pthread_exit(NULL);
}


void *starterThread(void *threadp)
{
   int i;

   syslog(LOG_INFO, "starter thread running on CPU=%d\n", sched_getcpu());

   for(i=0; i < NUM_THREADS; i++)
   {
       threadParams[i].threadIdx=i;

       pthread_create(&threads[i],   // pointer to thread descriptor
                      &fifo_sched_attr,     // use FIFO RT max priority attributes
                      counterThread, // thread function entry point
                      (void *)&(threadParams[i]) // parameters to pass in
                     );

   }

   for(i=0;i<NUM_THREADS;i++)
       pthread_join(threads[i], NULL);
   pthread_exit(NULL);
}


int main (int argc, char *argv[])
{
   int rc;
   int j;
   cpu_set_t cpuset;

   set_scheduler();

   CPU_ZERO(&cpuset);
   uname(&unameData);
   syslog(LOG_INFO, "[COURSE:1][ASSIGNMENT:4] %s %s %s %s %s GNU/Linux\n", 
	        unameData.sysname,
		unameData.nodename, 
	        unameData.release,
	     	unameData.version,
	   	unameData.machine);
   // get affinity set for main thread
   mainthread = pthread_self();

   // Check the affinity mask assigned to the thread 
   rc = pthread_getaffinity_np(mainthread, sizeof(cpu_set_t), &cpuset);
   if (rc != 0)
       perror("pthread_getaffinity_np");
   else
   {
       syslog(LOG_INFO, "main thread running on CPU=%d, CPUs =", sched_getcpu());

       for (j = 0; j < CPU_SETSIZE; j++)
           if (CPU_ISSET(j, &cpuset))
               syslog(LOG_INFO, " %d", j);

       syslog(LOG_INFO, "\n");
   }

   pthread_create(&startthread,   // pointer to thread descriptor
                  &fifo_sched_attr,     // use FIFO RT max priority attributes
                  starterThread, // thread function entry point
                  (void *)0 // parameters to pass in
                 );

   pthread_join(startthread, NULL);

   printf("\nTEST COMPLETE\n");
   EXIT_SUCCESS;
}
