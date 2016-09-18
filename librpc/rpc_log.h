#ifndef RPC_LOG_H_
#define RPC_LOG_H_

#define log(x) do { \
    printf("(%s:%d) %s\n", __func__, __LINE__, x); \
} while(0)

#endif
