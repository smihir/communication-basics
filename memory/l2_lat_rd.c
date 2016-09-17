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

#define RUNS 1024 * 4

// on 64-bit machines, addr lenght is 64bits, which is 8 bytes
// so STRIDE should not be less than 8bytes. Ever.
#define STRIDE 64
// POOl depends on l2 cache size, our l1 cache is 256KB so we
// will allocate 128KB pool so that it is always in cache
#define POOL 128 * 1024

#define ITERATIONS 10
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

uint64_t mean(uint64_t *v, int size) {
    int i = 0;
    uint64_t s = 0;

    for (i = 0; i < size; i++) {
        s += v[i];
    }
    return (s/size);
}

void calculate_latency(uint64_t *timetsc, size_t len) {
    int i;

    for (i = 0; i < ITERATIONS; i++) {
        printf("time for %d movs is %lu, "
               "time per mov %f\n",
                RUNS, timetsc[i],
                (double)(timetsc[i]) / (RUNS));
    }
}

int main(int argc, char **argv) {
    register int iterations = ITERATIONS;
    int warmup = WARMUP;
    int e;
    char *ll, *addr;
    struct timespec time1, time2;
    uint64_t timetsc[ITERATIONS];
    register char **p;
    register int i;
    register int count = (POOL / (STRIDE * 128)) + 1;

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

    sleep(1);
    while (warmup-- > 0) {
        for (i = 0; i < count; i++) {
            ONETWENTYEIGHT;
        }
    }

    // now that everything is in l2, load first 32KB in l1
    // STRIDE is 64b=8Bytes to put in 32KB l1 we need atleast
    // 32 * 1024 / 8 = 4096 movs
    p = (char **)&addr[0];
    KILO;
    KILO;
    KILO;
    KILO;
    KILO;

    while (iterations-- > 0) {
        clock_gettime(CLOCK_REALTIME, &time1);

        KILO;
        KILO;
        KILO;
        KILO;

        clock_gettime(CLOCK_REALTIME, &time2);
        timetsc[ITERATIONS - 1 - iterations] = timediff(&time1, &time2);
    }

    // don't let compile optimize the above ll traversals!
    dummy = (long)*p;

    free(ll);
    calculate_latency(timetsc, ITERATIONS);
    return 0;
}
