CC = gcc
CSCOPE = cscope
CFLAGS += -Wall -Werror
LDFLAGS += -lrt -lm

CLKPRECISIONOBJS := clock_precision.o

ifeq ($(DEBUG), y)
 CFLAGS += -g -DDEBUG
endif

ifeq ($(TSCTIMER), y)
 TIMERFLAGS += -DTSCTIMER
endif

.PHONY: all
all: clk_precision

clk_precision: $(CLKPRECISIONOBJS)
	$(CC) $(CFLAGS) $(CLKPRECISIONOBJS) -o $@ $(LDFLAGS)

%.o: %.c *.h
	$(CC) $(CFLAGS) $(TIMERFLAGS) -o $@ -c $<

cscope:
	$(CSCOPE) -bqR

.PHONY: clean
clean:
	rm -rf *.o clk_precision
