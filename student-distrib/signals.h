#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include "types.h"
typedef void (sighandler_t) ();

typedef enum {
    SIG_DIV_ZERO = 0,
    SIG_SEGFAULT,
    SIG_KB_INT,
    SIG_ALARM,
    SIG_USER0,
    SIG_SIZE,
} signal_t;

typedef struct {
    uint32_t regs[10];
    uint32_t irq_num;
    uint32_t error;
    void *addr;
    uint32_t cs;
    uint32_t eflags;
    void *esp;
    uint32_t ds;
} __attribute__ ((packed)) hw_context_t;

#define SIG_FLAG(s) (1 << s)

void check_signals(hw_context_t *context);
extern int sigreturn_linkage();

#endif
