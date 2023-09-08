// Sergey Stain, little lib for real time 

#ifndef STAIN_REALTIME_H
#define STAIN_REALTIME_H

#include <pthread.h> // for pthread_attr_t. Remember to compile and link with -pthread

// System calls errors handling
void rt_errorcall(const char* syscall_name);
void rt_errorret(const char* function_name, int errcode);

// Force CPU to use only maximum frequency
void rt_lockCPUSpeed();
void rt_unlockCPUSpeed();

// Printf current scheduler policy and priority
void rt_printf_sched();

// Get current thread priority negative offset form max
int rt_get_priority();

// Change scheduler policy and priority for the current thread
void rt_change_scheduler(int sched_policy, int neg_priority_offset);

// Set attributes for a RT pthreads creation
void rt_init_pthread_attr(pthread_attr_t* attr_p, int sched_policy, int neg_priority_offset, int cpu);

// Init clock
void rt_clock_init();

// Get relative time
struct timespec rt_clock();

// Calc time delta
double rt_delta(const struct timespec* stop, const struct timespec* start);

#endif // STAIN_REALTIME_H
