CC = gcc
CSCOPE = cscope
CFLAGS += -Wall -Werror -Winline

MEMOBJS := measurements.o
CACHEINFOOBJS := cache_info.o
L1LATRDOBJ := l1_lat_rd.o
L2LATRDOBJ := l2_lat_rd.o

ifeq ($(DEBUG), y)
 CFLAGS += -g -DDEBUG
endif

.PHONY: all
all: mem_measurements cache_info l1_lat_rd l2_lat_rd

mem_measurements: $(MEMOBJS)
	$(CC) $(CFLAGS) $(MEMOBJS) -o $@ $(LDFLAGS)

cache_info: $(CACHEINFOOBJS)
	$(CC) $(CFLAGS) $(CACHEINFOOBJS) -o $@ $(LDFLAGS)

l1_lat_rd: $(L1LATRDOBJ)
	$(CC) $(CFLAGS) $(L1LATRDOBJ) -o $@ $(LDFLAGS)

l2_lat_rd: $(L2LATRDOBJ)
	$(CC) $(CFLAGS) $(L2LATRDOBJ) -o $@ $(LDFLAGS)

%.o: %.c *.h
	$(CC) $(CFLAGS) $(TIMERFLAGS) -o $@ -c $<

cscope:
	$(CSCOPE) -bqR

.PHONY: clean
clean:
	rm -rf *.o mem_measurements cache_info l1_lat_rd l2_lat_rd
