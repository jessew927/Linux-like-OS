#ifndef _TASK_H_
#define _TASK_H_

#include "types.h"
#include "page.h"
#include "signals.h"

#define BUF_SIZE 256
// Maximum number of files open for each task
#define TASK_MAX_FILES 8
#define TASK_MAX_FD    7
// The starting page index for the first task = 8 MB
#define TASK_START_PAGE 2
#define TASK_PAGE_INDEX(c) (TASK_START_PAGE + c)

#define TASK_IMG_START_ADDR 0x08048000
// Kernel stack top for the task; Also location for PCB
#define TASK_KSTACK_TOP(c) (0x800000 - 0x2000 * (c + 1))
// Kernel stack bottom (start) for the task
#define TASK_KSTACK_BOT(c) (0x800000 - 0x2000 * (c))
// User stack bottom (start) for the task
#define TASK_USTACK_BOT(c) (0x800000 + 0x40000 * (c + 1))
#define KSTACK_TOP_MASK (~0x1FFF)

#define MAX_PROC_NUM 10

typedef enum {
    TASK_FILE_REG,
    TASK_FILE_DIR,
    TASK_FILE_RTC,
    TASK_FILE_TERM,
} task_file_flags_type_t;

typedef struct {
    task_file_flags_type_t type;
    uint8_t used;
} file_flags_t;

typedef struct {
    // Functions for operating on the file; in order of open, read, write, close
    struct file_ops_table *file_ops;
    int32_t inode;
    int32_t pos;
    file_flags_t flags;
} FILE;

typedef struct file_ops_table {
    int32_t (*open)(const int8_t* filename, FILE *file);
    int32_t (*read)(int8_t* buf, uint32_t nbytes, FILE *file);
    int32_t (*write)(const int8_t* buf, uint32_t nbytes, FILE *file);
    int32_t (*close)(FILE *file);
} file_ops_table_t;

typedef struct PCB_s {
    FILE open_files[TASK_MAX_FILES];
    struct PCB_s *parent;
    const int8_t *cmd_args;
    uint8_t *esp;
    uint8_t *ebp;
    int8_t signals;
    uint8_t pid;
    uint8_t term_ind;
    // Total number of objects; unused objects are counted
    uint32_t malloc_obj_count;
    sighandler_t *signal_handlers[SIG_SIZE];
} PCB_t;


#endif /* ifndef _TASK_H_ */
