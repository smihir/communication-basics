CC = gcc
CSCOPE = cscope
CFLAGS += -Wall -Werror -Winline
LDFLAGS += -lm

CLKPRECISIONOBJS := clock_gettime_precision.o
TSCPRECISIONOBJS := tsc_precision.o

ifeq ($(DEBUG), y)
 CFLAGS += -g -DDEBUG
endif

.PHONY: all
all: clk_precision tsc_precision

clk_precision: $(CLKPRECISIONOBJS)
	$(CC) $(CFLAGS) $(CLKPRECISIONOBJS) -o $@ $(LDFLAGS)

tsc_precision: $(TSCPRECISIONOBJS)
	$(CC) $(CFLAGS) $(TSCPRECISIONOBJS) -o $@ $(LDFLAGS)

%.o: %.c *.h
	$(CC) $(CFLAGS) $(TIMERFLAGS) -o $@ -c $<

cscope:
	$(CSCOPE) -bqR

.PHONY: clean
clean:
	rm -rf *.o clk_precision tsc_precision
