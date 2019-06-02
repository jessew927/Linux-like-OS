#include "syscall.h"
#include "term.h"
#include "lib.h"
#include "task.h"
#include "rtc.h"
#include "file_sys.h"
#include "x86_desc.h"

uint8_t pid_used[MAX_PROC_NUM] = {0};
malloc_obj_t *malloc_objs = (malloc_obj_t *) MALLOC_HEAP_MAP_START;

int32_t syscall_halt(uint8_t status) {
    return _syscall_halt(status, (hw_context_t *) (((uint32_t *) &status) + 3));
}

int32_t _syscall_halt(uint32_t status, hw_context_t *context) {
    // Revert info from PCB
    PCB_t *task_pcb = get_cur_pcb();
    PCB_t *parent_pcb = task_pcb->parent;
    if (!parent_pcb) {
        uint32_t entry_addr;
        entry_addr = *((int32_t *) (TASK_IMG_START_ADDR + ELF_ENTRY_OFFSET));
        context->addr = (void *) entry_addr;
        context->esp = (void *) TASK_VIRT_PAGE_END;
        return 0;
    }

    uint8_t i;
    for (i = 2; i < TASK_MAX_FILES; i ++) {
        if (task_pcb->open_files[i].flags.used) {
            task_pcb->open_files[i].file_ops->close(&task_pcb->open_files[i]);
        }
    }

    int32_t ppid = parent_pcb->pid;
    page_directory[USER_PAGE_INDEX].page_PDE.page_addr = TASK_PAGE_INDEX(ppid);
    tss.esp0 = TASK_KSTACK_BOT(ppid);
    // Reload the TLB
    asm volatile(
        "movl %%cr3, %%eax;"
        " movl %0, %%cr3; "
        :
        : "r"(page_directory)
        :"%eax"
    );

    pid_used[task_pcb->pid] = 0;
    terms[task_pcb->term_ind].cur_pid = ppid;
    uint32_t prev_ebp = (uint32_t) parent_pcb->ebp;
    uint32_t prev_esp = (uint32_t) parent_pcb->esp;
    asm volatile (
        "xor %%ebx, %%ebx;"
        "mov %0, %%ebx;"
        "mov %1, %%eax;"
        "mov %2, %%ecx;"
        "mov %%eax, %%ebp;"
        "mov %%ecx, %%esp;"
        "jmp 1f;"
        :
        : "m" (status), "m" (prev_ebp), "m" (prev_esp)
        : "%eax", "%ecx", "%ebx"
    );
    // unreachable!!!
    while (1) { asm volatile ("hlt;"); }
    return 0;
}

int32_t syscall_execute(const int8_t *command) {
    return _syscall_execute(command, -1);
}

int32_t _syscall_execute(const int8_t* command, int8_t term_ind) {
    int pid;
    for (pid = 1; pid < MAX_PROC_NUM; pid ++) {
        if (!pid_used[pid]) {
            goto syscall_execute__parse_args;
        }
    }
    return -1;      // No usable pid

syscall_execute__parse_args:;
    int i;
    for (i = 0; command[i] > ' '; i ++);
    const int8_t *args = command + i;

    // 1. Setup arguments
    while (*args == ' ') {
        args ++;
    }
    if (*args == 0) {
        args = 0;
    }

    // 2. Copy the command itself so we have a guaranteed null-terminated string
    // Also copy the arguments
    int8_t filename[i + 1];
    strncpy(filename, command, i + 1);
    filename[i] = 0;

    uint8_t arg_len = args ? strlen(args) : 0;
    int8_t copied_args[arg_len + 1];
    if (args) {
        strncpy(copied_args, args, arg_len + 1);
        copied_args[arg_len] = 0;
    }

    int32_t entry_addr;
    // 3. Check executable format and load task image
    FILE f;
    if (fs_open(filename, &f) == -1) {
        return -1;
    }

    int8_t buf[BUF_SIZE];
    int32_t read_size = fs_file_read(buf, BUF_SIZE, &f);
    // Check for ELF magic
    if (buf[0] != 0x7F || buf[1] != 'E' || buf[2] != 'L' || buf[3] != 'F') {
        fs_file_close(&f);
        return -1;
    }

    entry_addr = *((int32_t *) (buf + ELF_ENTRY_OFFSET));
    // Check that the entry address is after the image starting point
    if (entry_addr < TASK_IMG_START_ADDR) {
        fs_file_close(&f);
        return -1;
    }

    // 4. Setup paging; Set task's target page address
    page_directory[USER_PAGE_INDEX].page_PDE.page_addr = TASK_PAGE_INDEX(pid);
    // Reload the TLB
    asm volatile(
        " movl %0, %%cr3; "
        :
        : "r"(page_directory)
    );

    // Copy the rest of the image
    uint8_t *task_img_cur = (uint8_t *) TASK_IMG_START_ADDR;
    memcpy(task_img_cur, buf, read_size);
    task_img_cur += read_size;
    while ((read_size = fs_file_read(buf, BUF_SIZE, &f)) > 0) {
        memcpy(task_img_cur, buf, read_size);
        task_img_cur += read_size;
    }

    fs_file_close(&f);

    // 5. Setup PCB
    PCB_t *cur_pcb = get_cur_pcb();
    PCB_t *task_pcb = (PCB_t *) TASK_KSTACK_TOP(pid);
    // Open stdin & stdout
    task_pcb->open_files[0].flags.used = 1;
    task_pcb->open_files[0].flags.type = TASK_FILE_TERM;
    task_pcb->open_files[0].file_ops = &stdin_file_ops_table;
    task_pcb->open_files[1].flags.used = 1;
    task_pcb->open_files[1].flags.type = TASK_FILE_TERM;
    task_pcb->open_files[1].file_ops = &stdout_file_ops_table;

    for (i = 2; i < TASK_MAX_FILES; i ++) {
        task_pcb->open_files[i].flags.used = 0;
    }

    task_pcb->cmd_args = args ? copied_args : NULL;
    asm volatile ("movl %%ebp, %0;" : "=r" (cur_pcb->ebp));
    asm volatile ("movl %%esp, %0;" : "=r" (cur_pcb->esp));
    if (cur_pcb != (PCB_t *) TASK_KSTACK_TOP(0) && term_ind == -1) {
        task_pcb->parent = cur_pcb;
    } else {
        task_pcb->parent = NULL;
    }
    task_pcb->pid = pid;
    task_pcb->signals = 0;
    task_pcb->malloc_obj_count = 1;
    task_pcb->term_ind = term_ind != -1 ? term_ind : cur_pcb->term_ind;
    malloc_objs[0].used = 0;
    malloc_objs[0].size = MALLOC_HEAP_SIZE;

    if (term_ind != -1) {
        terms[(int) term_ind].cur_pid = pid;
    } else {
        terms[cur_pcb->term_ind].cur_pid = pid;
    }

    tss.esp0 = TASK_KSTACK_BOT(pid);
    tss.ss0 = KERNEL_DS;

    pid_used[pid] = 1;

    for (i = 0; i < SIG_SIZE; i ++) {
        task_pcb->signal_handlers[i] = NULL;
    }

    // 6. Context switch
    asm volatile (
        "movl %0, %%eax;"  // User DS
        "movw %%ax, %%ds;"
        "movw %%ax, %%es;"
        "movw %%ax, %%fs;"
        "movw %%ax, %%gs;"
        "push %%eax;"
        "movl %1, %%eax;" // User ESP
        "push %%eax;"
        "pushf;"
        "orl $0x200, (%%esp);"
        "movl %2, %%eax;" // User CS
        "push %%eax;"
        "movl %3, %%eax;" // Entry point
        "push %%eax;"
        "iret;"
"1:;"
        :
        : "r" (USER_DS), "r" (TASK_VIRT_PAGE_END), "r" (USER_CS), "r" (entry_addr)
        : "%eax"
    );

    int32_t retval;
    asm volatile ("movl %%ebx, %0;" : "=r" (retval));
    return retval;
}

int32_t syscall_read(int32_t fd, void *buf, uint32_t nbytes) {
    if (!buf) {
        return -1;
    }

    if (fd < 0 || fd >= TASK_MAX_FILES) {
        return -1;
    }

    PCB_t *task_pcb = get_cur_pcb();
    if (!task_pcb->open_files[fd].flags.used) {
        return -1;
    }
    return task_pcb->open_files[fd].file_ops->read(
            buf, nbytes, &task_pcb->open_files[fd]);
}

int32_t syscall_write(int32_t fd, const void *buf, uint32_t nbytes) {
    if (!buf) {
        return -1;
    }

    if (fd < 0 || fd >= TASK_MAX_FILES) {
        return -1;
    }

    PCB_t *task_pcb = get_cur_pcb();
    if (!task_pcb->open_files[fd].flags.used) {
        return -1;
    }
    return task_pcb->open_files[fd].file_ops->write(
            buf, nbytes, &task_pcb->open_files[fd]);
}

/* syscall_open
 *  Descrption: Add file descriptor on a process's PCB
 *
 *  Arg:
 *      filename: name of the target file
 *
 * 	RETURN:
 *      file descriptor number, which is a small non-negative number,
 *      0 if this file is an RTC file
 *      -1 if failed.
 */
int32_t syscall_open(const int8_t* filename) {
    dentry_t dent;
    if( read_dentry_by_name(filename, &dent) != 0 ){
        return -1;
    }

    // Need a curent pid to get the right PCB
    PCB_t *task_pcb = get_cur_pcb();
    int i;
    // determine the type of file
    for (i = 2; i < TASK_MAX_FILES; i ++) {
        if (!(task_pcb->open_files[i].flags.used)) {
            int32_t retval;
            if (dent.filetype == FILE_TYPE_RTC) {
                retval = rtc_open(filename, &task_pcb->open_files[i]);
            } else {
                retval = fs_open(filename, &task_pcb->open_files[i]);
            }
            if (retval) {
                return -1;
            }
            task_pcb->open_files[i].flags.used = 1;
            return i;
        }
    }
    return -1;
}

int32_t syscall_close(int32_t fd) {
    if (fd < 0 || fd >= TASK_MAX_FILES) {
        return -1;
    }

    PCB_t *task_pcb = get_cur_pcb();
    if (!task_pcb->open_files[fd].flags.used) {
        return -1;
    }

    if (task_pcb->open_files[fd].file_ops->close(&task_pcb->open_files[fd])) {
        return -1;
    }

    task_pcb->open_files[fd].flags.used = 0;
    return 0;
}

int32_t syscall_getargs(int8_t* buf, uint32_t nbytes) {
    if (!buf) {
        return -1;
    }
    PCB_t *task_pcb = get_cur_pcb();
    if (!task_pcb->cmd_args) {
        return -1;
    }

    if (nbytes < strlen(task_pcb->cmd_args) + 1) {
        return -1;
    }
    strcpy(buf, task_pcb->cmd_args);
    return 0;
}

int32_t syscall_vidmap(uint8_t** screen_start) {
    if ((uint32_t) screen_start < TASK_VIRT_PAGE_BEG
            || (uint32_t) screen_start >= TASK_VIRT_PAGE_END) {
        return -1;
    }

    /*PCB_t *task_pcb = get_cur_pcb();
    if (task_pcb->term_ind == cur_term_ind) {*/
        *screen_start = (uint8_t *) TASK_VIDMEM_START;
    /*} else {
        *screen_start = terms[(int) task_pcb->term_ind].video_mem;
    }*/
    return (int32_t) *screen_start;
}

int32_t syscall_set_handler(int32_t signum, void* handler) {
    if (signum < 0 || signum > SIG_SIZE) {
        return -1;
    }

    PCB_t *task_pcb = get_cur_pcb();
    task_pcb->signal_handlers[signum] = handler;
    return 0;
}

int32_t _syscall_sigreturn(hw_context_t *context) {
    uint32_t *user_esp = context->esp;
    hw_context_t *saved_context = (hw_context_t *) user_esp;
    context->esp = saved_context->esp;
    context->addr = saved_context->addr;
    uint8_t i;
    for (i = 0; i < 10; i ++) {
        context->regs[i] = saved_context->regs[i];
    }
    return 0;
}

uint8_t *syscall_malloc(uint32_t size) {
    uint8_t *obj_ptr = (uint8_t *) HEAP_START;
    int16_t i;
    uint32_t total_empty_size = 0;
    PCB_t *task_pcb = get_cur_pcb();
    for (i = 0; i < task_pcb->malloc_obj_count; i ++) {
        uint32_t obj_block_size = malloc_objs[i].size;
        uint32_t obj_size = obj_block_size * MALLOC_BLOCK_SIZE;
        if (!malloc_objs[i].used) {
            if (size < obj_size) {
                if (task_pcb->malloc_obj_count == MALLOC_OBJ_NUM) {
                    return NULL;
                }
                int16_t j;
                for (j = task_pcb->malloc_obj_count; j >= i; j --) {
                    malloc_objs[j + 1] = malloc_objs[j];
                }
                malloc_objs[i].used = 1;
                malloc_objs[i].size = (size / 32) + 1;
                malloc_objs[i + 1].size = obj_block_size - malloc_objs[i].size;
                task_pcb->malloc_obj_count ++;
                return obj_ptr;
            }
            if (size > obj_size) {
                total_empty_size += obj_size;
                goto probe_next_obj;
            }

            malloc_objs[i].used = 1;
            return obj_ptr;
        }
probe_next_obj:
        obj_ptr += malloc_objs[i].size * MALLOC_BLOCK_SIZE;
    }

    return NULL;
}

int32_t syscall_free(uint8_t *ptr) {
    if (!ptr) {
        return 0;
    }
    uint8_t *obj_ptr = (uint8_t *) HEAP_START;
    uint16_t i;
    PCB_t *task_pcb = get_cur_pcb();
    for (i = 0; i < task_pcb->malloc_obj_count && ptr > obj_ptr; i ++) {
        obj_ptr += malloc_objs[i].size * MALLOC_BLOCK_SIZE;
    }
    if (obj_ptr != ptr) {
        return -1;
    }

    uint16_t empty_size = malloc_objs[i].size;
    uint8_t move = 0;
    if (i < task_pcb->malloc_obj_count && !malloc_objs[i + 1].used) {
        move ++;
        empty_size += malloc_objs[i + 1].size;
    }
    if (i > 0 && malloc_objs[i - 1].used == 0) {
        move ++;
        empty_size += malloc_objs[i + 1].size;
        i --;
    }

    task_pcb->malloc_obj_count -= move;
    uint16_t j;
    for (j = i + 1; j < task_pcb->malloc_obj_count; j ++) {
        malloc_objs[j] = malloc_objs[j + move];
    }
    malloc_objs[i].used = 0;
    malloc_objs[i].size = empty_size;

    return 0;
}

PCB_t *get_cur_pcb() {
    uint32_t cur_esp;
    asm volatile ("movl %%esp, %0;" : "=r" (cur_esp));
    return (PCB_t *) (cur_esp & KSTACK_TOP_MASK);
}

int32_t do_syscall(int32_t call, int32_t a, int32_t b, int32_t c) {
    int32_t ret;
    asm volatile (
        "mov %1, %%eax;"
        "mov %2, %%ebx;"
        "mov %3, %%ecx;"
        "mov %4, %%edx;"
        "int $0x80;"
        "mov %%eax, %0;"
        : "=g" (ret)
        : "g" (call), "g" (a), "g" (b), "g" (c)
        : "%eax", "%ebx", "%ecx", "%edx"
    );
    return ret;
}
