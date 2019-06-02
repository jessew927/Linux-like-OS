#define ASM     1

#include "idt.h"
#include "rtc.h"

.globl _de_isr
.globl _db_isr
.globl _nmi_isr
.globl _bp_isr
.globl _of_isr
.globl _br_isr
.globl _ud_isr
.globl _nm_isr
.globl _df_isr
.globl _cso_isr
.globl _ts_isr
.globl _np_isr
.globl _ss_isr
.globl _gp_isr
.globl _pf_isr
.globl _mf_isr
.globl _ac_isr
.globl _mc_isr
.globl _xf_isr

.globl _pit_isr
.globl _kb_isr
.globl _cas_isr
.globl _com2_isr
.globl _com1_isr
.globl _lpt2_isr
.globl _flop_isr
.globl _lpt1_isr
.globl _rtc_isr
.globl _free1_isr
.globl _free2_isr
.globl _free3_isr
.globl _ps2m_isr
.globl _fpu_isr
.globl _hd1_isr
.globl _hd2_isr

.globl _syscall_isr
.globl sigreturn_linkage

SYSCALL_JMP_TAB:
    .long syscall_halt
    .long syscall_execute
    .long syscall_read
    .long syscall_write
    .long syscall_open
    .long syscall_close
    .long syscall_getargs
    .long syscall_vidmap
    .long syscall_set_handler
    .long syscall_sigreturn
    .long syscall_malloc
    .long syscall_free

# Interrupt 1st level handlers
PIC_ISR_jmp_tab:
    .long pit_isr           // 0     Programmable Interrupt Timer Interrupt
    .long kb_isr            // 1     Keyboard Interrupt
    .long 0                 // 2     Cascade (used internally by the two PICs. never raised)
    .long 0                 // 3     COM2 (if enabled)
    .long 0                 // 4     COM1 (if enabled)
    .long 0                 // 5     LPT2 (if enabled)
    .long 0                 // 6     Floppy Disk
    .long 0                 // 7     LPT1 / Unreliable "spurious" interrupt (usually)
    .long rtc_isr           // 8     CMOS real-time clock (if enabled)
    .long 0                 // 9     Free for peripherals / legacy SCSI / NIC
    .long 0                 // 10     Free for peripherals / SCSI / NIC
    .long 0                 // 11     Free for peripherals / SCSI / NIC
    .long 0                 // 12     PS2 Mouse
    .long 0                 // 13     FPU / Coprocessor / Inter-processor
    .long 0                 // 14     Primary ATA Hard Disk
    .long 0                 // 15     Secondary ATA Hard Disk

# Syscall
_syscall_isr:
    push $0
    push $0
    jmp common_isr

syscall_sigreturn:
    // Skip the return addr
    lea 4(%esp), %eax
    push %eax
    call _syscall_sigreturn
    add $4, %esp
    mov 28(%esp), %eax
    ret

sigreturn_linkage:
    add $4, %esp
    mov $10, %eax
    int $0x80

common_isr:
    push %fs
    push %es
    push %ds
    push %eax
    push %ebp
    push %edi
    push %esi
    push %edx
    push %ecx
    push %ebx
    mov %esp, %ebp

    cmp $0, 40(%ebp)
    jg common_isr__handle_exception
    jl common_isr__handle_pic

common_isr__handle_syscall:
    cmp $1, %eax
    jl common_isr__syscall_error
    cmp $12, %eax
    jg common_isr__syscall_error
    sub $1, %eax
    mov SYSCALL_JMP_TAB(, %eax, 4), %eax
    call *%eax
common_isr__switch_end:
    mov %eax, 24(%ebp)
    jmp common_isr__return

common_isr__syscall_error:
    movl $-1, 24(%ebp)
    jmp common_isr__return

common_isr__handle_exception:
    push 44(%ebp)
    push 40(%ebp)
    sub $1, (%esp)
    call exception_handler
    add $8, %esp
    jmp common_isr__return

common_isr__handle_pic:
    mov 40(%ebp), %eax
    neg %eax
    sub $1, %eax
    push %eax
    mov PIC_ISR_jmp_tab(, %eax, 4), %eax
    call *%eax
    pop %eax
    cmpl $8, %eax
    jge common_isr__pic_slave_eoi
    mov $0x20, %al
    outb %al, $0x20
    jmp common_isr__return

common_isr__pic_slave_eoi:
    mov $0x20, %al
    outb %al, $0x20
    outb %al, $0xA0
    jmp common_isr__return

common_isr__return:
    mov %esp, %eax
    push %eax
    call check_signals
    add $4, %esp

    pop %ebx
    pop %ecx
    pop %edx
    pop %esi
    pop %edi
    pop %ebp
    pop %eax
    pop %ds
    pop %es
    pop %fs
    add $8, %esp
    iret



_pit_isr:    // 0     Programmable Interrupt Timer Interrupt
    push $0
    push $-1
    jmp common_isr

_kb_isr:    // 1     Keyboard Interrupt
    push $0
    push $-2
    jmp common_isr

_cas_isr:    // 2     Cascade (used internally by the two PICs. never raised)
    push $0
    push $-3
    jmp common_isr

_com2_isr:    // 3     COM2 (if enabled)
    push $0
    push $-4
    jmp common_isr

_com1_isr:    // 4     COM1 (if enabled)
    push $0
    push $-5
    jmp common_isr

_lpt2_isr:    // 5     LPT2 (if enabled)
    push $0
    push $-6
    jmp common_isr

_flop_isr:    // 6     Floppy Disk
    push $0
    push $-7
    jmp common_isr

_lpt1_isr:    // 7     LPT1 / Unreliable "spurious" interrupt (usually)
    push $0
    push $-8
    jmp common_isr

_rtc_isr:    // 8     CMOS real-time clock (if enabled)
    push $0
    push $-9
    jmp common_isr

_free1_isr:    // 9     Free for peripherals / legacy SCSI / NIC
    push $0
    push $-10
    jmp common_isr

_free2_isr:    // 10     Free for peripherals / SCSI / NIC
    push $0
    push $-11
    jmp common_isr

_free3_isr:    // 11     Free for peripherals / SCSI / NIC
    push $0
    push $-12
    jmp common_isr

_ps2m_isr:    // 12     PS2 Mouse
    push $0
    push $-13
    jmp common_isr

_fpu_isr:    // 13     FPU / Coprocessor / Inter-processor
    push $0
    push $-14
    jmp common_isr

_hd1_isr:    // 14     Primary ATA Hard Disk
    push $0
    push $-15
    jmp common_isr

_hd2_isr:    // 15     Secondary ATA Hard Disk
    push $0
    push $-16
    jmp common_isr

# Exception 1st level handler
# See IA-32 Manual p.145 for error code presence
_de_isr:
    push $0
    push $1
    jmp common_isr

_db_isr:
    push $0
    push $2
    jmp common_isr

_nmi_isr:
    push $0
    push $3
    jmp common_isr

_bp_isr:
    push $0
    push $4
    jmp common_isr

_of_isr:
    push $0
    push $5
    jmp common_isr

_br_isr:
    push $0
    push $6
    jmp common_isr

_ud_isr:
    push $0
    push $7
    jmp common_isr

_nm_isr:
    push $0
    push $8
    jmp common_isr

_df_isr:
    push $9
    jmp common_isr

_cso_isr:
    push $0
    push $10
    jmp common_isr

_ts_isr:
    push $11
    jmp common_isr

_np_isr:
    push $12
    jmp common_isr

_ss_isr:
    push $13
    jmp common_isr

_gp_isr:
    push $14
    jmp common_isr

_pf_isr:
    push $15
    jmp common_isr

_mf_isr:
    push $0
    push $16
    jmp common_isr

_ac_isr:
    push $17
    jmp common_isr

_mc_isr:
    push $0
    push $18
    jmp common_isr

_xf_isr:
    push $0
    push $19
    jmp common_isr
