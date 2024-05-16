// Timer Interrupt handler

#include "include/types.h"
#include "include/param.h"
#include "include/riscv.h"
#include "include/sbi.h"
#include "include/spinlock.h"
#include "include/timer.h"
#include "include/printf.h"
#include "include/proc.h"
#include "include/syscall.h"
#include "include/vm.h"

struct spinlock tickslock;
uint ticks;

void timerinit() {
    initlock(&tickslock, "time");
    #ifdef DEBUG
    printf("timerinit\n");
    #endif
}

void
set_next_timeout() {
    // There is a very strange bug,
    // if comment the `printf` line below
    // the timer will not work.

    // this bug seems to disappear automatically
    // printf("");
    sbi_set_timer(r_time() + INTERVAL);
}

void timer_tick() {
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
    set_next_timeout();
}

//add for syscall
uint64 sys_gettimeofday() {

    #ifdef DEBUG
    printf("Entering gettimeofday\n");
    printf("r_time, INTERVAL: %d, %d\n", r_time(), INTERVAL);
    #endif

    uint64 addr;
    int tz;
    if(argaddr(0, &addr) < 0 || argint(1, &tz) < 0)
        return -1;
    TimeVal tm;
    uint tmp = r_time();
    uint64 usecs = tmp * R_TIME_TIMES;
    tm.sec = usecs/1000000;
    tm.usec = usecs%1000000;
    if(copyout2(addr, (char *) &tm, sizeof(tm)) < 0)
        return -1;
    return 0;
}