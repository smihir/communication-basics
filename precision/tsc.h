#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <string.h>

inline uint64_t _rdtsc() {
    unsigned long int lo, hi;
    __asm__ __volatile__ ("rdtscp" : "=a" (lo), "=d" (hi));
    return (uint64_t)hi << 32 | lo;
}

static double tsc_frequency()
{
    FILE* cpuinfo;
    char str[1000];
    double cpu_mhz = 0;

    cpuinfo = fopen("/proc/cpuinfo","r");
    while(fgets(str,1000,cpuinfo) != NULL){
        char cmp_str[8];
        strncpy(cmp_str, str, 7);
        cmp_str[7] = '\0';
        if (strcmp(cmp_str, "cpu MHz") == 0) {
            sscanf(str, "cpu MHz : %lf", &cpu_mhz);
            break;
        }
    }
    fclose(cpuinfo);
    return cpu_mhz;
}
#endif
