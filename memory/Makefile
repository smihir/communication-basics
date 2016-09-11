CC = gcc
CSCOPE = cscope
CFLAGS += -Wall -Werror -Winline

MEMOBJS := measurements.c

ifeq ($(DEBUG), y)
 CFLAGS += -g -DDEBUG
endif

.PHONY: all
all: mem_measurements

mem_measurements: $(MEMOBJS)
	$(CC) $(CFLAGS) $(MEMOBJS) -o $@ $(LDFLAGS)

%.o: %.c *.h
	$(CC) $(CFLAGS) $(TIMERFLAGS) -o $@ -c $<

cscope:
	$(CSCOPE) -bqR

.PHONY: clean
clean:
	rm -rf *.o clk_precision tsc_precision
