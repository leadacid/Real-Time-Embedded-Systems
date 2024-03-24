#define _GNU_SOURCE  // for Linux specific CPU_* macros, gettid 
#include <errno.h>   // for errno
#include <error.h>   // for error()
#include <pthread.h> // for pthread_*. Compile and link with -pthread
#include <sched.h>   // for sched_* 
#include <stdio.h>   // for printf
#include <stdlib.h>  // for exit
#include <string.h>  // for strerror_r
#include <syslog.h>  // for syslog
#include <sys/types.h> // for getpid, gettid
#include <unistd.h>    // for getpid, gettid

#include "realtime.h"

//static const int kClock = CLOCK_REALTIME;
//static const int kClock = CLOCK_MONOTONIC;
static const int kClock = CLOCK_MONOTONIC_RAW;
//static const int kClock = CLOCK_REALTIME_COARSE;
//static const int kClock = CLOCK_MONOTONIC_COARSE;

static struct timespec init_stamp = {0,0};

// Init clock
void rt_clock_init() {
    if (clock_gettime(kClock, &init_stamp) != 0) rt_errorcall("clock_gettime");
}
// Calc time delta
static struct timespec timespec_delta(const struct timespec* stop, const struct timespec* start) {
	struct timespec dt = {stop->tv_sec - start->tv_sec, stop->tv_nsec - start->tv_nsec};
	if (dt.tv_nsec > 0 && dt.tv_sec < 0) {
		dt.tv_nsec -= 1000000000;
		++dt.tv_sec;	
	} else if (dt.tv_nsec < 0 && dt.tv_sec > 0) {
		dt.tv_nsec += 1000000000;
		--dt.tv_sec;
	} else {
		// OK
	}
	return dt;
}
// Get relative time
struct timespec rt_clock() {
    struct timespec cur_stamp;
    if (clock_gettime(kClock, &cur_stamp) != 0) rt_errorcall("clock_gettime");
    return timespec_delta(&cur_stamp, &init_stamp);
}
// Calc floating point time delta
double rt_delta(const struct timespec* stop, const struct timespec* start) {
    struct timespec dt = timespec_delta(stop, start);
    return dt.tv_sec + dt.tv_nsec / 1000000000.0;
}

// Force CPU to use only maximum frequency
void rt_lockCPUSpeed() {
    system("sudo cp /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq "
		"/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
}
void rt_unlockCPUSpeed() {
    system("sudo cp /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq "
		"/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
}

// System calls errors handling
void rt_errorcall(const char* syscall_name) {
   rt_errorret(syscall_name, errno);
}
void rt_errorret(const char* function_name, int errcode) {
    syslog(LOG_CRIT, "Error ocurred when calling %s. Programm will terminate.",
            function_name);
    error(1, errcode, "ERROR calling %s", function_name);
}

// Get name string for scheduler policy
static char* policy_name(int policy) {
	switch (policy) {
        case SCHED_FIFO: return "SCHED_FIFO";
        case SCHED_OTHER: return "SCHED_OTHER";
        case SCHED_RR: return "SCHED_RR";
		case SCHED_IDLE: return "SCHED_IDLE";
		case SCHED_BATCH: return "SCHED_BATCH";
        default: return "UNKNOWN";
    }
}

// Printf current scheduler policy and priority
void rt_printf_sched() {
	int policy;
    struct sched_param param;
    int rc = pthread_getschedparam(pthread_self(), &policy, &param);
    if (rc != 0) rt_errorret("pthread_getschedparam", rc);
	printf("threadid=%d %s prior=%d on CPU%d\n", 
           gettid(), policy_name(policy), param.sched_priority, sched_getcpu());
}

// Get current thread priority negative offset form max
int rt_get_priority() {
	int policy;
    struct sched_param param;
    int rc = pthread_getschedparam(pthread_self(), &policy, &param);
    if (rc != 0) rt_errorret("pthread_getschedparam", rc);
    return param.sched_priority - sched_get_priority_max(policy);
}


// Change scheduler policy and priority for the current thread
void rt_change_scheduler(int policy, int neg_priority_offset) {
	const int pid = getpid();
	const int old_policy = sched_getscheduler(pid);
	const int priority = sched_get_priority_max(policy) + neg_priority_offset;
	
	struct sched_param new_param;
	new_param.sched_priority = priority;
	
    int rc = pthread_setschedparam(pthread_self(), policy, &new_param);
    if (rc != 0) rt_errorret("pthread_setschedparam", rc);
	
	const int new_policy = sched_getscheduler(pid);
	syslog(LOG_DEBUG, "Process_id=%d changed its scheduler from %s to %s prior=%d",
            pid, policy_name(old_policy), policy_name(new_policy), priority);
}

// Set attributes for a RT pthreads creation
void rt_init_pthread_attr(pthread_attr_t* attr_p, int policy, int neg_priority_offset, int cpu) {
    int rc = pthread_attr_init(attr_p); 
    if (rc != 0) rt_errorret("pthread_attr_init", rc);
    // Force to use our setting instead of ignorig them and inherit them from a parent
    rc = pthread_attr_setinheritsched(attr_p, PTHREAD_EXPLICIT_SCHED);
    if (rc != 0) rt_errorret("pthread_attr_setinheritsched", rc);
    // Set CPU affinity
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    rc = pthread_attr_setaffinity_np(attr_p, sizeof(cpu_set_t), &cpuset);
    if (rc != 0) rt_errorret("pthread_attr_setaffinity_np", rc);
    // Set policy
    rc = pthread_attr_setschedpolicy(attr_p, policy);
    if (rc != 0) rt_errorret("pthread_attr_setschedpolicy", rc);
    // Set priority
    struct sched_param fifo_param;
    fifo_param.sched_priority = sched_get_priority_max(policy) + neg_priority_offset;
    rc = pthread_attr_setschedparam(attr_p, &fifo_param);
    if (rc != 0) rt_errorret("pthread_attr_setschedparam", rc);
}


