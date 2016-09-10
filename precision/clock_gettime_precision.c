#define _GNU_SOURCE
#include <math.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_RUNS  10
#define NS_IN_SEC 1000000000UL

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

    set_affinity(0);

    for (i = 0; i < MAX_RUNS; i++) {
        struct timespec time1, time2;
        sleep(1);
        clock_gettime(CLOCK_MONOTONIC_RAW, &time1);
        clock_gettime(CLOCK_MONOTONIC_RAW, &time2);
        timetsc[i] = timediff(&time1, &time2);
    }

    meanval = mean(timetsc, MAX_RUNS);
    sdval = stdev(timetsc, MAX_RUNS);
    printf("Raw Precision: mean %lu stdev %f\n", meanval, sdval);

    //calculating accuracy
    for (i = 0; i < MAX_RUNS; i++) {
        struct timespec time1, time2;
        clock_gettime(CLOCK_MONOTONIC_RAW, &time1);
        sleep(1);
        clock_gettime(CLOCK_MONOTONIC_RAW, &time2);
        timetsc[i] = timediff(&time1, &time2);
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
