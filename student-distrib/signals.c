#include "signals.h"
#include "syscall.h"
#include "x86_desc.h"
#include "lib.h"

void check_signals(hw_context_t *context) {
    PCB_t *task_pcb = get_cur_pcb();
    if (!task_pcb->signals) {
        return;
    }

    // Only deliver signal when return to user
    if (context->cs != USER_CS) {
        return;
    }

    uint8_t i;
    for (i = 0; i < SIG_SIZE; i ++) {
        uint8_t mask = SIG_FLAG(i);
        if (task_pcb->signals & mask) {
            task_pcb->signals &= ~mask;
            if (task_pcb->signal_handlers[i]) {
                uint32_t *user_esp = context->esp;
                user_esp -= sizeof(hw_context_t) / sizeof(uint32_t);
                hw_context_t *saved_context = (hw_context_t *) user_esp;
                *saved_context = *context;

                user_esp -= 1;
                *user_esp = (uint32_t) i;
                user_esp -= 1;
                *user_esp = (uint32_t) &sigreturn_linkage;
                context->esp = user_esp;
                context->addr = task_pcb->signal_handlers[i];
                return;
            }

            if (i < 3) {
                _syscall_halt(256, context);
            }
            // Ignore others
        }
    }
}
