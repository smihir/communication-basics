#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <string.h>

#define _rdtsc(ticks) do { \
    volatile unsigned long int lo, hi; \
    __asm__ __volatile__ ("rdtscp" : "=a" (lo), "=d" (hi)); \
    ticks = (uint64_t)hi << 32 | lo; \
} while(0)

static double tsc_frequency(CPUID)
{
    FILE* cpuinfo;
    char str[100];
    double cpu_khz = 0;
    char fname[100];
    snprintf(fname, sizeof(fname), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq",
        CPUID);

    cpuinfo = fopen(fname,"r");
    while(fgets(str,sizeof(str),cpuinfo) != NULL){
        sscanf(str, "%lf", &cpu_khz);
        break;
    }
    fclose(cpuinfo);
    return cpu_khz / 1000;
}
#endif
