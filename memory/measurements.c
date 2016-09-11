#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sched.h>

// version 1: from http://stackoverflow.com/questions/21369381/measuring-cache-latencies
//            fails with segmatation fault
// version 2: add text from intel manual for quick verification of code


struct cache_info {
    int cache_type;
    int cache_level;
    int cache_sets;
    int cache_coherency_line_size;
    int cache_physical_line_partitions;
    int cache_ways_of_associativity;
    int cache_total_size;
    int cache_is_fully_associative;
    int cache_is_self_initializing;
};

void set_affinity(int cpuid) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpuid, &set);

    if (sched_setaffinity(getpid(), sizeof(set), &set) == -1)
        perror("sched_affinity");
}

int i386_cpuid_caches (struct cache_info *info, size_t info_size) {
    int i;
    int num_caches = 0;
    struct cache_info *cinfo;
    for (i = 0; i < info_size; i++) {
        cinfo = &info[i];

        //04H NOTES:
        //    Leaf 04H output depends on the initial value in ECX.*
        //    See also: “INPUT EAX = 04H: Returns Deterministic Cache Parameters for Each Level” on page 213.
        //EAX Bits 04 - 00: Cache Type Field.
        //    0 = Null - No more caches.
        //    1 = Data Cache.
        //    2 = Instruction Cache.
        //    3 = Unified Cache.
        //    4-31 = Reserved.
        //    Bits 07 - 05: Cache Level (starts at 1).
        //    Bit 08: Self Initializing cache level (does not need SW initialization).
        //    Bit 09: Fully Associative cache.
        //    Bits 13 - 10: Reserved.
        //    Bits 25 - 14: Maximum number of addressable IDs for logical processors sharing this cache**, ***.
        //    Bits 31 - 26: Maximum number of addressable IDs for processor cores in the physical
        //    package**, ****, *****.
        //EBX Bits 11 - 00: L = System Coherency Line Size**.
        //    Bits 21 - 12: P = Physical Line partitions**.
        //    Bits 31 - 22: W = Ways of associativity**.
        //ECX Bits 31-00: S = Number of Sets**.
        //EDX Bit 00: Write-Back Invalidate/Invalidate.
        //    0 = WBINVD/INVD from threads sharing this cache acts upon lower level caches for threads sharing this
        //    cache.
        //    1 = WBINVD/INVD is not guaranteed to act upon lower level caches of non-originating threads sharing
        //    this cache.
        //    Bit 01: Cache Inclusiveness.
        //    0 = Cache is not inclusive of lower cache levels.
        //    1 = Cache is inclusive of lower cache levels.
        //    Bit 02: Complex Cache Indexing.
        //    0 = Direct mapped cache.
        //    1 = A complex function is used to index the cache, potentially using all address bits.
        //    Bits 31 - 03: Reserved = 0.
        //NOTES:
        //* If ECX contains an invalid sub leaf index, EAX/EBX/ECX/EDX return 0. Sub-leaf index n+1 is invalid if subleaf
        //* n returns EAX[4:0] as 0.
        //* ** Add one to the return value to get the result.
        //* ***The nearest power-of-2 integer that is not smaller than (1 + EAX[25:14]) is the number of unique initial
        //* APIC IDs reserved for addressing different logical processors sharing this cache.
        //* **** The nearest power-of-2 integer that is not smaller than (1 + EAX[31:26]) is the number of unique
        //* Core_IDs reserved for addressing different processor cores in a physical package. Core ID is a subset of
        //* bits of the initial APIC ID.
        //* ***** The returned value is constant for valid initial values in ECX. Valid ECX values start from 0.

        //INPUT EAX = 04H: Returns Deterministic Cache Parameters for Each Level
        //    When CPUID executes with EAX set to 04H and ECX contains an index value, the processor returns encoded data
        //    that describe a set of deterministic cache parameters (for the cache level associated with the input in ECX). Valid
        //    index values start from 0.
        //    Software can enumerate the deterministic cache parameters for each level of the cache hierarchy starting with an
        //    index value of 0, until the parameters report the value associated with the cache type field is 0. The architecturally
        //    defined fields reported by deterministic cache parameters are documented in Table 3-8.
        //    This Cache Size in Bytes
        //    = (Ways + 1) * (Partitions + 1) * (Line_Size + 1) * (Sets + 1)
        //    = (EBX[31:22] + 1) * (EBX[21:12] + 1) * (EBX[11:0] + 1) * (ECX + 1)
        //    The CPUID leaf 04H also reports data that can be used to derive the topology of processor cores in a physical
        //    package. This information is constant for all valid index values. Software can query the raw data reported by
        //    executing CPUID with EAX=04H and ECX=0 and use it as part of the topology enumeration algorithm described in
        //    Chapter 8, “Multiple-Processor Management,” in the Intel® 64 and IA-32 Architectures Software Developer’s
        //    Manual, Volume 3A.


        // Variables to hold the contents of the 4 i386 legacy registers
        uint32_t eax, ebx, ecx, edx;

        eax = 4; // get cache info
        ecx = i; // cache id

        asm (
            "cpuid" // call i386 cpuid instruction
            : "+a" (eax) // contains the cpuid command code, 4 for cache query
            , "=b" (ebx)
            , "+c" (ecx) // contains the cache id
            , "=d" (edx)
        ); // generates output in 4 registers eax, ebx, ecx and edx 

        // taken from http://download.intel.com/products/processor/manual/325462.pdf Vol. 2A 3-149
        int cache_type = eax & 0x1F; 

        if (cache_type == 0) // end of valid cache identifiers
            break;

        char * cache_type_string;
        switch (cache_type) {
            case 1: cache_type_string = "Data Cache"; break;
            case 2: cache_type_string = "Instruction Cache"; break;
            case 3: cache_type_string = "Unified Cache"; break;
            default: cache_type_string = "Unknown Type Cache"; break;
        }

        int cache_level = (eax >>= 5) & 0x7;

        int cache_is_self_initializing = (eax >>= 3) & 0x1; // does not need SW initialization
        int cache_is_fully_associative = (eax >>= 1) & 0x1;


        // taken from http://download.intel.com/products/processor/manual/325462.pdf 3-166 Vol. 2A
        // ebx contains 3 integers of 10, 10 and 12 bits respectively
        unsigned int cache_sets = ecx + 1;
        unsigned int cache_coherency_line_size = (ebx & 0xFFF) + 1;
        unsigned int cache_physical_line_partitions = ((ebx >>= 12) & 0x3FF) + 1;
        unsigned int cache_ways_of_associativity = ((ebx >>= 10) & 0x3FF) + 1;

        // Total cache size is the product
        size_t cache_total_size = cache_ways_of_associativity * cache_physical_line_partitions * cache_coherency_line_size * cache_sets;

        //if (cache_type == 1 || cache_type == 3) {
        //    data_caches[num_data_caches++] = cache_total_size;
        //}
        num_caches++;

        printf(
            "Cache ID %d:\n"
            "- Level: %d\n"
            "- Type: %s\n"
            "- Sets: %d\n"
            "- System Coherency Line Size: %d bytes\n"
            "- Physical Line partitions: %d\n"
            "- Ways of associativity: %d\n"
            "- Total Size: %zu bytes (%zu kb)\n"
            "- Is fully associative: %s\n"
            "- Is Self Initializing: %s\n"
            "\n"
            , i
            , cache_level
            , cache_type_string
            , cache_sets
            , cache_coherency_line_size
            , cache_physical_line_partitions
            , cache_ways_of_associativity
            , cache_total_size, cache_total_size >> 10
            , cache_is_fully_associative ? "true" : "false"
            , cache_is_self_initializing ? "true" : "false"
        );

        cinfo->cache_type = cache_type;
        cinfo->cache_level = cache_level;
        cinfo->cache_sets = cache_sets;
        cinfo->cache_coherency_line_size = cache_coherency_line_size;
        cinfo->cache_physical_line_partitions = cache_physical_line_partitions;
        cinfo->cache_ways_of_associativity = cache_ways_of_associativity;
        cinfo->cache_total_size = cache_total_size;
        cinfo->cache_is_fully_associative = cache_is_fully_associative;
        cinfo->cache_is_self_initializing = cache_is_self_initializing;
    }

    return num_caches;
}

int test_l1_cache(size_t attempts, size_t cache_size, int * latencies, size_t max_latency) {
    uint64_t * random_data;
    //random_data = mmap(
    //      NULL
    //    , cache_size
    //    , PROT_READ | PROT_WRITE
    //    , MAP_PRIVATE | MAP_ANON // | MAP_POPULATE
    //    , -1
    //    , 0
    //    ); // get some random data
    //if (random_data == MAP_FAILED) {
    //    perror("mmap");
    //    abort();
    //}
    int e = posix_memalign((void **)&random_data, sysconf(_SC_PAGESIZE), cache_size);
    if (e != 0) {
        perror("failed to allocate memory:");
        exit(1);
    }

    sleep(1);

    // bring in L1
    size_t i;
    for (i = 0; i < cache_size / sizeof(random_data[0]); i += 1) {
        random_data[i] = 1;
    }

    int64_t random_offset = 0;
    random_offset += rand();
    random_offset %= cache_size;
    uint64_t *loc = random_data + random_offset;
    while (attempts--) {
        int32_t temp1, temp2;
        uint64_t cycles_used, edx, eax;
        asm (
            "mfence\n\t"        // memory fence
            "rdtsc\n\t"         // get cpu cycle count
            "mov %%edx, %2\n\t"
            "mov %%eax, %3\n\t"
            "mfence\n\t"        // memory fence
            "mov %4, %%al\n\t"  // load data
            "mfence\n\t"
            "rdtsc\n\t"
            "sub %3, %%eax\n\t" // substract cycle count
            "sbb %2, %%edx"     // substract cycle count
            : "=a" (eax)
            , "=d" (edx)
            , "=r" (temp1)
            , "=r" (temp2)
            : "m" (loc)
            );
        cycles_used = edx << 32 | eax;
        if (cycles_used < max_latency)
            latencies[cycles_used]++;
        else 
            latencies[max_latency - 1]++;
    }

    //munmap(random_data, cache_size);
    free(random_data);


    return 0;
} 

int test_l2l3_cache(size_t attempts, size_t cache_size, int * latencies, size_t max_latency) {
    uint64_t * random_data;
    int e = posix_memalign((void **)&random_data, sysconf(_SC_PAGESIZE), cache_size);
    if (e != 0) {
        perror("failed to allocate memory:");
        exit(1);
    }

    sleep(1);

    size_t i;
    // all data is in l2 now
    // last l1_cache_size bytes of data are in l1
    // first byte is in l2
    for (i = 0; i < cache_size / sizeof(random_data[0]); i += 1) {
        random_data[i] = 1;
    }

    while (attempts--) {
        // attempts is so small as compared to cache sizes that we are
        // reading from l2l3
        uint64_t *loc = random_data + attempts;
        int32_t temp1, temp2;
        uint64_t cycles_used, edx, eax;
        asm (
            "mfence\n\t"        // memory fence
            "rdtsc\n\t"         // get cpu cycle count
            "mov %%edx, %2\n\t"
            "mov %%eax, %3\n\t"
            "mfence\n\t"        // memory fence
            "mov %4, %%al\n\t"  // load data
            "mfence\n\t"
            "rdtsc\n\t"
            "sub %3, %%eax\n\t" // substract cycle count
            "sbb %2, %%edx"     // substract cycle count
            : "=a" (eax)
            , "=d" (edx)
            , "=r" (temp1)
            , "=r" (temp2)
            : "m" (loc)
            );
        cycles_used = edx << 32 | eax;
        if (cycles_used < max_latency)
            latencies[cycles_used]++;
        else 
            latencies[max_latency - 1]++;
    }

    //munmap(random_data, cache_size);
    free(random_data);


    return 0;
} 
int main() {
    struct cache_info info[32];
    int num_caches = i386_cpuid_caches(info, sizeof(info));

    int latencies[0x400];
    memset(latencies, 0, sizeof(latencies));

    int empty_cycles = 0;

    int i;
    // has to be a lot less than difference in subsequent
    // cache sizes otherwise measurement will not work
    int attempts = 10000;
    set_affinity(0);
    for (i = 0; i < attempts; i++) { // measure how much overhead we have for counting cyscles
        int32_t temp1, temp2;
        uint64_t cycles_used, edx, eax;
        asm (
            "mfence\n\t"        // memory fence
            "rdtscp\n\t"         // get cpu cycle count
            "mov %%edx, %2\n\t"
            "mov %%eax, %3\n\t"
            "mfence\n\t"        // memory fence
            "mfence\n\t"
            "rdtsc\n\t"
            "sub %3, %%eax\n\t" // substract cycle count
            "sbb %2, %%edx"     // substract cycle count
            : "=a" (eax)
            , "=d" (edx)
            , "=r" (temp1)
            , "=r" (temp2)
            :
            );
        cycles_used = edx << 32 | eax;
        if (cycles_used < sizeof(latencies) / sizeof(*latencies))
            latencies[cycles_used]++;
        else 
            latencies[sizeof(latencies) / sizeof(*latencies) - 1]++;

    }

    {
        int j;
        size_t sum = 0;
        for (j = 0; j < sizeof(latencies) / sizeof(*latencies); j++) {
            sum += latencies[j];
        }
        size_t sum2 = 0;
        for (j = 0; j < sizeof(latencies) / sizeof(*latencies); j++) {
            sum2 += latencies[j];
            if (sum2 >= sum * .75) {
                empty_cycles = j;
                fprintf(stderr, "Empty counting takes %d cycles\n", empty_cycles);
                break;
            }
        }
    }

    for (i = 0; i < num_caches; i++) {
        if (info[i].cache_type == 1 || info[i].cache_type == 3) {
            memset(latencies, 0, sizeof(latencies));
            if (info[i].cache_level == 1) {
                test_l1_cache(attempts, info[i].cache_total_size, latencies, sizeof(latencies) / sizeof(*latencies));
            } else {
                test_l2l3_cache(attempts, info[i].cache_total_size, latencies, sizeof(latencies) / sizeof(*latencies));
           }
        } else {
            continue;
        }

        int j;
        size_t sum = 0;
        for (j = 0; j < sizeof(latencies) / sizeof(*latencies); j++) {
            sum += latencies[j];
        }
        size_t sum2 = 0;
        for (j = 0; j < sizeof(latencies) / sizeof(*latencies); j++) {
            sum2 += latencies[j];
            if (sum2 >= sum * .75) {
                fprintf(stderr, "Cache ID %i has latency %d cycles\n", i, j - empty_cycles);
                break;
            }
        }

    }

    return 0;

}
