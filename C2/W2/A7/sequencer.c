// Based on the code example "sequencer_generic/seqgen3" by Dr. Sam Siewert 

#define _GNU_SOURCE // for sched_getcpu
#include <pthread.h> // for pthread_*. Need to compile and link with -pthread
#include <sched.h>   // for sched_* 
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h> 
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>  

#include "realtime.h" // My lib with all the handy for real time functions

// Services workload functinons prototypes
void service1(void);
void service2(void);
void service3(void);
//***** CONFIG *****************************************************************
//
static const char* kSyslogPrefixStr = "[COURSE:2][ASSIGNMENT:7]";
// To realize RM scheduler, all services threads should run on the same cpu core
static const int kServicesCPU = 3;
// The sequencer can really run on any core (it usually have low WCET)
// But if the sequencer thread will run on the same core,
// it should have priority higher then any service.
// We better give the hiest priority for the sequencer anyway.
static const int kSequencerPrior = 0; // 0 is the highest, -1 -2 etc. are lower
// The scheduler time unit defines a sequencer calling period
static const int kSequencerTickMilliSec = 10; 
// We can run the sequencer for a long time untill a user input 
//  or just for the fisrt LCM period
static const bool kStopAtLCM = false;
// Synthetic load value with computation time  equivalent to 1ms
static const int kSyntheticLoad1ms = 897600; // 1800MHz, -o2 or -o3 compilation

// Services params table
static const struct {
    void(*handler)(void);  // Service workload function
    int period;            // Service period in number of kSequencerTickMilliSec
    int priority;          // Service priority negative offset
    int cpu;               // CPU core index to run onto
    float capacity;          // Service capacity in ticks
} services_params[] = { 
    {service1,  3, -1, kServicesCPU, 1}, 
    {service2,  6, -2, kServicesCPU, 2}, 
    {service3, 9, -3, kServicesCPU, 3},
};
// LCM of all periods should be calculated: and used as a wraparound time point
static const int kLCMPeriod = 18; // sequence will repeat each kLCMPeriod ticks
//
//******************************************************************************

// Total services number is calced atomaticaly from the params table
#define SERVICE_NUM  (sizeof(services_params)/sizeof(services_params[0]))

// Service control data type and global variables
typedef struct {
    int index;            // Service index
    bool abortReq;        // Request flag to abort a service thread
    sem_t semaphore;      // Semaphore to make a service request
} ServiceControls;
static ServiceControls services_controls[SERVICE_NUM];

// Global timer to periodically call sequencer
static timer_t timer;

// Global int to detect preemptions
static int last_started_prior = 0; // Priority of the last started thread

// CPU load with stable computation time = ticks * tickPeriod
void syntheticLoad(const float ticks) {
    volatile long int result = 0;// "volatile" prevents code skipping
    register long int a = 1; // "register" prevents alocating in RAM 
    register long int b = 2;
    const register int N = kSyntheticLoad1ms * kSequencerTickMilliSec * ticks;
    for (register int i = 0; i <= N; ++i) {
        register long int temp_b = b; 
        b += a; // will overflow quickly, but values are unused and don't matter
        a = temp_b;
    }
    result += b;// "+=" prevents "unused variable" warning
}

// Common control routine for all services 
// and an entry point for services threads
// It gets a pointer to controls for a paticular service  
void* service_control_routine(void* inp_p) {
    ServiceControls* controls_p = inp_p; // controls for a particular service
    const int thread_prior = rt_get_priority();
    printf("Service %d will run as ", controls_p->index + 1); 
    rt_printf_sched(); 

    while(true) { 
        // Wait for service request from the sequencer through a semaphore
        sem_wait(&controls_p->semaphore); // block in pending state
        // Check for synchronous abort request
        if (controls_p->abortReq) break;
        // Got a service request, so run it one more time
        last_started_prior = thread_prior;
        // Call the service handler to do work
        (services_params[controls_p->index].handler)();
        // Check for inversed priority preemption
        if (last_started_prior < thread_prior) {
            syslog(LOG_INFO, "ERROR! Higher priority(%d) service was preempted."
                   " with lower one (%d).", thread_prior, last_started_prior);
        }
        // Check the service deadline (new semaphore shouldn't be given)
        int sem_val;
        if (sem_getvalue(&controls_p->semaphore, &sem_val) != 0) {
            rt_errorcall("sem_getvalue");
        }
        if (sem_val > 0) {
            syslog(LOG_INFO, "WARN! Service %d miss the deadline!", 
                    controls_p->index+1);
        } 
    }
    syslog(LOG_INFO, "Service %d was aborted.", controls_p->index + 1);
    return NULL;
}

// Service workload functions
void service1(void) {
    static long int cnt = 0;
    ++cnt;
    const struct timespec start = rt_clock();
    syntheticLoad(services_params[0].capacity);
    const struct timespec stop = rt_clock();
    syslog(LOG_DEBUG, "Thread 1 start %4ld @ %ld.%09ld on core %d", 
            cnt, start.tv_sec, start.tv_nsec, sched_getcpu());
    // Check the computation time
    const double c = rt_delta(&stop, &start);
    const double target_c = services_params[0].capacity 
        * kSequencerTickMilliSec / 1000.0; 
    if (c > 1.005*target_c || c < 0.995*target_c) {
        syslog(LOG_INFO, "WARN! Thread 1 computation takes %lf sec", c);
    }
}
void service2(void) {
    static long int cnt = 0;
    ++cnt;
    const struct timespec start = rt_clock();
    syntheticLoad(services_params[1].capacity); 
    syslog(LOG_DEBUG, "Thread 2 start %4ld @ %ld.%09ld on core %d",  
            cnt, start.tv_sec, start.tv_nsec, sched_getcpu());
}
void service3(void) {
    static long int cnt = 0;
    ++cnt;
    const struct timespec start = rt_clock();
    syntheticLoad(services_params[2].capacity);
    syslog(LOG_DEBUG, "Thread 3 start %4ld @ %ld.%09ld on core %d",  
            cnt, start.tv_sec, start.tv_nsec, sched_getcpu());
}

// Sequencer will post semaphores according to services_params
//  It should be calls every time unit (kSequencerTickMilliSec) by the timer
void sequencer(int signum) {
    static int tick = -1;
    // Print some information at the very first tick
    if (tick == -1) { 
        rt_clock_init(); // init my lib clock (set it to zero)
        printf("Sequencer func is running as "); rt_printf_sched(); 
    }
    // Print at each first tick of LCM period
    if (tick == 0) {

        const struct timespec time = rt_clock();
        syslog(LOG_DEBUG, "Sequencer start a new LCM period @ %ld.%09ld",
                time.tv_sec, time.tv_nsec);
    }
    // Check the period for each service
    for (int i = 0; i < SERVICE_NUM; ++i) {
        if (tick % services_params[i].period == 0) {
            // It's time for a new request of this service
            sem_t* sem_p = &(services_controls[i].semaphore);
            // Make a new service request 
            sem_post(sem_p);
        }
    }
    // Count a new sequenser tick and wraparound if needeed  
    ++tick;
    if (tick >= kLCMPeriod) { 
        tick = 0;
		if (kStopAtLCM) {
			// Abort all the service threads
			syslog(LOG_INFO, "Abortion is requested by timeout.");
			for (int i = 0; i < SERVICE_NUM; ++i) {
				services_controls[i].abortReq = true;
			}
		}
	}
    // Check the timer for an overrun
    int over_cnt = timer_getoverrun(timer);
    if (over_cnt > 0) syslog(LOG_INFO, "ERROR! Sequencer timer got overrun.");
}

static void output_uname_command_to_syslog() {
    FILE* pipe = popen("uname -a", "r");
    if (pipe) {
        char buf[128];
        if (fgets(buf, sizeof(buf), pipe)) {
            syslog(LOG_INFO, "%s", buf);
        }
        pclose(pipe);
    } else {
        rt_errorcall("popen");
    }
}

int main (int argc, char* argv[]) {
    printf("Don't forget to call as ROOT using sudo\n"); 
    printf("\nTEST STARTED\n");

	// Open syslog using kPrefixSting as an ident
    // Using LOG_NDELAY makes the first syslog() call faster
	openlog(kSyslogPrefixStr, LOG_NDELAY, LOG_USER);
    
    // Print the output of "uname -a" shell command to the syslog
    syslog(LOG_INFO, " ") ;
    output_uname_command_to_syslog();
    syslog(LOG_INFO, "Programm get started");

    // This permits realtime processes to use more then 95% of a CPU
	// Use to test service sets with low margin.
	// But it's DANGEROUS and can starve the kernel and lock up a system!
	system("echo -1 > /proc/sys/kernel/sched_rt_runtime_us");

    // Disable energy saving CPU frequencies
    rt_lockCPUSpeed();
        
    printf("Main thread is running as ");	rt_printf_sched(); 

    // Initialize services control data and check parameters
    for (int i = 0; i < SERVICE_NUM; ++i) {
        services_controls[i].index = i;
        services_controls[i].abortReq = false;
        if (sem_init(&services_controls[i].semaphore, 0, 0) != 0) {
            rt_errorcall("sem_init");
        }
		// Check the LCM period value
		if (kLCMPeriod % services_params[i].period != 0) {
			printf("Error! LCM period is definitely wrong.");	
			exit(EXIT_FAILURE);
		}
    }

    // Create services threads
    pthread_t service_threads[SERVICE_NUM]; 
    for (int i = 0; i < SERVICE_NUM; ++i) {
        // Tune the thread parameters
        pthread_attr_t th_attr;
        rt_init_pthread_attr(&th_attr, SCHED_FIFO, 
                services_params[i].priority, services_params[i].cpu);
        // Create the thread and it will run immediately
        int rc = pthread_create(
                  &service_threads[i],
                  &th_attr,     // individual attributes
                  service_control_routine, // common entry point
                  &services_controls[i]// individual control set
                 );
        if (rc != 0) rt_errorret("pthread_create", rc);
        // Initialized pthread_attr_t should be destroyed
        rc = pthread_attr_destroy(&th_attr);
        if (rc != 0) rt_errorret("pthread_attr_destroy", rc); 
    }    
  
    // Sequencer func will be run from the main thread
	// So change scheduler params for the main thread to RT high priority
	rt_change_scheduler(SCHED_FIFO, kSequencerPrior);

    printf("Main thread is now running as "); rt_printf_sched(); 

    // Create Sequencer, which like a cyclic executive, has the highest prio
    printf("Start the sequencer\n");

    // Create timer that will set up SIGALRM signal 
    // I use CLOCK_MONOTONIC to prevent time changing glitches
    // Yet, the manual sais that CLOCK_REALTIME is OK and should not give
    // glitches for relative (not absolute) intervals
    if (timer_create(CLOCK_MONOTONIC, NULL, &timer) != 0) {
        rt_errorcall("timer_create");
    }
    // Dispose the sequencer as a handler function for SIGALRM signal
    if (signal(SIGALRM, sequencer) == SIG_ERR) rt_errorcall("signal");
    // Arm the timer with periodic reloadable interval
    const struct timespec tick_time = {0, kSequencerTickMilliSec * 1000000};
    const struct itimerspec itimer = {tick_time, tick_time};
    timer_settime(timer, 0, &itimer, NULL);

    // Wait for one period for the timer can proc once
    nanosleep(&tick_time, NULL); // can wakes erlier, but it's OK

    if (!kStopAtLCM) {
		// Wait for user to stop the programm
		printf("\nServices are running. Press ENTER to stop...\n");
		(void)getchar();
        const struct timespec t = rt_clock();
		syslog(LOG_INFO, "Abortion is requested by user @ %ld.%09ld", 
                t.tv_sec, t.tv_nsec);
        for (int i = 0; i < SERVICE_NUM; ++i) {
            services_controls[i].abortReq = true;
        }
	}
	// Join all the service threads
    for (int i = 0; i < SERVICE_NUM; ++i) {
        // Wait each service thread to complete
        int rc = pthread_join(service_threads[i], NULL);
        if (rc != 0) rt_errorret("main pthread_join", rc);
    }
    // Stop and delete sequencer timer
    if (timer_delete(timer) != 0) rt_errorcall("timer_delete");
  
    // Enable energy saving CPU frequencies
    rt_unlockCPUSpeed();

    printf("Syslog file is updated. See /var/log/syslog\n");
    printf("TEST COMPLETE\n");
}
