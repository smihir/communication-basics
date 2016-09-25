#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>

#define NS_IN_SEC 1000000000UL

#define ONE p = (char **)*p; // mov (%rbx),%rbx
#define FIVE ONE ONE ONE ONE ONE
#define TEN FIVE FIVE
#define FIFTY TEN TEN TEN TEN TEN
#define HUNDRED FIFTY FIFTY
#define ONETWENTYEIGHT HUNDRED TEN TEN FIVE ONE ONE ONE
#define TWOFIFTYSIX ONETWENTYEIGHT ONETWENTYEIGHT
#define FIVEONETWO TWOFIFTYSIX TWOFIFTYSIX 
#define KILO FIVEONETWO FIVEONETWO

#define K2 KILO KILO
#define K4 K2 K2
#define K8 K4 K4
#define K16 K8 K8
#define K32 K16 K16
#define K64 K32 K32
#define K128 K64 K64

#define RUNS 1024 * 128

// on 64-bit machines, addr lenght is 64bits, which is 8 bytes
// so STRIDE should not be less than 8bytes. Ever.
#define STRIDE 64
// POOl depends on l2 cache size, our l1 cache is 256KB so we
// will allocate 128KB pool so that it is always in cache
#define POOL 256 * 1024

#define ITERATIONS 10000
#define INNERLOOP  10000
#define WARMUP 5

static volatile uint64_t dummy;

void set_affinity(int cpuid) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpuid, &set);

    if (sched_setaffinity(getpid(), sizeof(set), &set) == -1)
        perror("sched_affinity");
}

uint64_t timediff(struct timespec *time1, struct timespec *time2) {
    uint64_t time1ns = (time1->tv_sec * NS_IN_SEC) + time1->tv_nsec;
    uint64_t time2ns = (time2->tv_sec * NS_IN_SEC) + time2->tv_nsec;

    return time2ns - time1ns;
}

int main(int argc, char **argv) {
    register int iterations = ITERATIONS;
    int warmup = WARMUP;
    int e;
    char *ll, *addr;
    struct timespec time1, time2;
    uint64_t timetsc[1];
    register char **p;
    register int i;
    register uint64_t looptsc;

    set_affinity(0);

    e = posix_memalign((void **)&ll, sysconf(_SC_PAGESIZE), POOL);
    if (e != 0) {
        perror("failed to allocate memory");
        exit(1);
    }
    addr = ll;

    for (i = STRIDE; i < POOL; i += STRIDE) {
        *(char **)&addr[i - STRIDE] = (char *)&addr[i];
    }
    // make circular list
    *(char **)&addr[i - STRIDE] = (char *)&addr[0];

    p = (char **)&addr[0];

    clock_gettime(CLOCK_REALTIME, &time1);
    while (iterations--) {
        for (i = 0; i < INNERLOOP; i++) {
        }
    }
    clock_gettime(CLOCK_REALTIME, &time2);
    looptsc = timediff(&time1, &time2);

    while (warmup-- > 0) {
        K32;
    }

    // now that everything is in l2, load first 32KB in l1
    // STRIDE is 64b=8Bytes to put in 32KB l1 we need atleast
    // 32 * 1024 / 8 = 4096 movs
    p = (char **)&addr[0];
    K4;

    iterations = ITERATIONS;
    clock_gettime(CLOCK_REALTIME, &time1);
    while (iterations--) {
        for (i = 0; i < INNERLOOP; i++) {
            ONE;
        }
    }
    clock_gettime(CLOCK_REALTIME, &time2);
    timetsc[0] = timediff(&time1, &time2);
    printf("%f\n", (double)(timetsc[0] - looptsc) / (ITERATIONS * INNERLOOP));

    // don't let compile optimize the above ll traversals!
    dummy = (long)*p;

    free(ll);
    //calculate_latency(timetsc, ITERATIONS);
    return 0;
}
