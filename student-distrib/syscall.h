#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "types.h"
#include "task.h"
#include "signals.h"

// Each block has a size of 32-bytes, and the allocator will only allocate
// multiples of blocks
// Total amount of heap is 256K, so it doesn't collide with program code
// A size of 0 means it has *1* block; used blocks can be ajacent to each
// other (as they represent deifferent allocated objects), but unused
// blocks can't. Each object can only have 1 block.
#define MALLOC_MAP_SIZE 4096
#define MALLOC_HEAP_SIZE (256 * 1024 / 32)
#define MALLOC_OBJ_NUM 2048
#define MALLOC_BLOCK_SIZE 32
#define MALLOC_HEAP_MAP_START (TASK_VIRT_PAGE_BEG + PCB_SIZE)
#define HEAP_START (TASK_VIRT_PAGE_BEG + PCB_SIZE + MALLOC_MAP_SIZE)
#define PCB_SIZE sizeof(PCB_t)

#define ELF_ENTRY_OFFSET 24
typedef struct {
    uint16_t used : 1;
    uint16_t size : 15;
} __attribute__((packed)) malloc_obj_t;

int32_t syscall_halt(uint8_t status);
int32_t _syscall_halt(uint32_t status, hw_context_t *context);
int32_t _syscall_execute(const int8_t* command, int8_t term_ind);
int32_t syscall_execute(const int8_t *command);
int32_t syscall_read(int32_t fd, void *buf, uint32_t nbytes);
int32_t syscall_write(int32_t fd, const void *buf, uint32_t nbytes);
int32_t syscall_open(const int8_t *filename);
int32_t syscall_close(int32_t fd);
int32_t syscall_getargs(int8_t *buf, uint32_t nbytes);
int32_t syscall_vidmap(uint8_t **screen_start);
int32_t syscall_set_handler(int32_t signum, void *handler);
extern int32_t syscall_sigreturn(void);
int32_t _syscall_sigreturn(hw_context_t *context);
uint8_t *syscall_malloc(uint32_t size);
int32_t syscall_free(uint8_t *ptr);
PCB_t *get_cur_pcb();
int32_t do_syscall(int32_t call, int32_t a, int32_t b, int32_t c);
int32_t init_proc(const int8_t* command, int8_t term_ind);

#endif
