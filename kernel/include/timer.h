#ifndef __TIMER_H
#define __TIMER_H

#include "types.h"
#include "spinlock.h"

extern struct spinlock tickslock;
extern uint ticks;

void timerinit();
void set_next_timeout();
void timer_tick();
typedef struct
{
    uint64 sec;  // 自 Unix 纪元起的秒数
    uint64 usec; // 微秒数
} TimeVal;

// factor to multiply r_time() to us in test
#define R_TIME_TIMES 1.2

struct tms
{
	long tms_utime;
	long tms_stime;
	long tms_cutime;
	long tms_cstime;
};
#endif
