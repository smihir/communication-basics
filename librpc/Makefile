CC = gcc
AR = ar
CSCOPE = cscope
CFLAGS += -Wall -Werror -Winline


ifeq ($(DEBUG), y)
 CFLAGS += -g -DDEBUG
endif


.PHONY: all
all: rpc_clnt.a rpc_svc.a test_clnt test_svc

rpc_clnt.a: rpc_clnt.o
	$(AR) rcs $@ $^

rpc_svc.a: rpc_svc.o
	$(AR) rcs $@ $^

test_clnt: test_clnt.o rpc_clnt.a
	$(CC) $(CFLAGS) test_clnt.o -L $(PWD) -l:/rpc_clnt.a -o $@

test_svc: test_svc.o rpc_svc.a
	$(CC) $(CFLAGS) test_svc.o -L $(PWD) -l:/rpc_svc.a -o $@

%.o: %.c *.h
	$(CC) $(CFLAGS) $(TIMERFLAGS) -o $@ -c $<

cscope:
	$(CSCOPE) -bqR

.PHONY: clean
clean:
	rm -rf *.o *.a test_clnt
