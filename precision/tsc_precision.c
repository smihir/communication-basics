#define _GNU_SOURCE
#include <math.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tsc.h"

#define CPUID 0
#define MAX_RUNS  10
#define NS_IN_SEC 1000000000UL

void set_affinity(int cpuid) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpuid, &set);

    if (sched_setaffinity(getpid(), sizeof(set), &set) == -1)
        perror("sched_affinity");
}

uint64_t timediff(uint64_t ticks1, uint64_t ticks2) {
    uint64_t tickdiff = ticks2 - ticks1;
    double tscfreq = tsc_frequency(CPUID);

    return (uint64_t)(((double)tickdiff / tscfreq) * 1000);
}

uint64_t mean(uint64_t *v, int size) {
    int i = 0;
    uint64_t s = 0;

    for (i = 0; i < size; i++) {
        s += v[i];
    }
    return (s/size);
}

double stdev(uint64_t *v, int size) {
    int i;
    uint64_t sq = 0, meanval = mean(v, size);

    for (i = 0; i < size; i++) {
        sq += ((v[i] - meanval) * (v[i] - meanval));
    }
    return (sqrt(((double)sq)/size));
}

int main()
{
    uint64_t timetsc[MAX_RUNS], meanval;
    double sdval;
    int i;

    set_affinity(CPUID);

    for (i = 0; i < MAX_RUNS; i++) {
        uint64_t ticks1, ticks2;
        sleep(1);
        _rdtsc(ticks1);
        _rdtsc(ticks2);
        timetsc[i] = timediff(ticks1, ticks2);
    }

    meanval = mean(timetsc, MAX_RUNS);
    sdval = stdev(timetsc, MAX_RUNS);
    printf("Raw Precision: mean %lu stdev %f\n", meanval, sdval);

    //calculating accuracy
    for (i = 0; i < MAX_RUNS; i++) {
        uint64_t ticks1, ticks2;
        _rdtsc(ticks1);
        sleep(1);
        _rdtsc(ticks2);
        timetsc[i] = timediff(ticks1, ticks2);
        // Assuming sleep of 1 sec is accurate
        if (timetsc[i] < NS_IN_SEC)
            timetsc[i] = NS_IN_SEC - timetsc[i];
        else
            timetsc[i] -= NS_IN_SEC;
    }

    meanval = mean(timetsc, MAX_RUNS);
    sdval = stdev(timetsc, MAX_RUNS);
    printf("Raw Error: mean %lu stdev %f percent error %f\n",
        meanval, sdval, ((double)meanval / NS_IN_SEC) * 100);

    return 0;
}
