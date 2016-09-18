CC = gcc
AR = ar
CSCOPE = cscope
CFLAGS += -Wall -Werror -Winline


ifeq ($(DEBUG), y)
 CFLAGS += -g -DDEBUG
endif


.PHONY: all
all: rpc_clnt.a test_clnt

rpc_clnt.a: rpc_clnt.o
	$(AR) rcs $@ $^

test_clnt: test_clnt.o rpc_clnt.a
	$(CC) $(CFLAGS) test_clnt.o -l:rpc_clnt.a -o $@


%.o: %.c *.h
	$(CC) $(CFLAGS) $(TIMERFLAGS) -o $@ -c $<

cscope:
	$(CSCOPE) -bqR

.PHONY: clean
clean:
	rm -rf *.o *.a test_clnt
