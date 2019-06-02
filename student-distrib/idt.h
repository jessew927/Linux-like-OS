#ifndef _IDT_H
#define _IDT_H

#include "types.h"

#define SYSCALL_IDX     0x80

// Interrupt indexes
#define PIT_INT     0x20
#define KB_INT			0x21
#define SLAVE_PIC_INT	0x22
#define RTC_INT 		0x28

#ifndef ASM

void exception_handler(uint32_t irq_num, uint32_t errorcode);

/* initializes the idt array */
extern void idt_init(void);

// Exception asm handlers
extern int _de_isr(void);
extern int _db_isr(void);
extern int _nmi_isr(void);
extern int _bp_isr(void);
extern int _of_isr(void);
extern int _br_isr(void);
extern int _ud_isr(void);
extern int _nm_isr(void);
extern int _df_isr(void);
extern int _cso_isr(void);
extern int _ts_isr(void);
extern int _np_isr(void);
extern int _ss_isr(void);
extern int _gp_isr(void);
extern int _pf_isr(void);
extern int _db_isr(void);
extern int _mf_isr(void);
extern int _ac_isr(void);
extern int _mc_isr(void);
extern int _xf_isr(void);

// External interrupt asm handlers
extern int _pit_isr(void);
extern int _kb_isr(void);
extern int _cas_isr(void);
extern int _com2_isr(void);
extern int _com1_isr(void);
extern int _lpt2_isr(void);
extern int _flop_isr(void);
extern int _lpt1_isr(void);
extern int _rtc_isr(void);
extern int _free1_isr(void);
extern int _free2_isr(void);
extern int _free3_isr(void);
extern int _ps2m_isr(void);
extern int _fpu_isr(void);
extern int _hd1_isr(void);
extern int _hd2_isr(void);

extern int _syscall_isr(void);

#endif /* ASM */

#endif
