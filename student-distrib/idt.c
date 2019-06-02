#include "idt.h"
#include "x86_desc.h"
#include "lib.h"
#include "task.h"
#include "signals.h"
#include "syscall.h"
#include "term.h"

void exception_handler(uint32_t irq_num, uint32_t errorcode) {
    if (irq_num == 14) {    // PF
        uint32_t addr;
        asm volatile ("movl %%cr2, %0;" : "=r" (addr));
        printf(terms, "exception: irq: %u, error: %u, addr: 0x%#x\n", irq_num, errorcode, addr);
    } else {
        printf(terms, "exception: irq: %u, error: %u\n", irq_num, errorcode);
    }
    PCB_t *task_pcb = get_cur_pcb();
    if (!irq_num) {     // Divide by zero
        task_pcb->signals |= SIG_FLAG(SIG_DIV_ZERO);
    } else {
        task_pcb->signals |= SIG_FLAG(SIG_SEGFAULT);
    }
}

/* Function creates everything as interrupt gates as recommended by descriptor doc
 * "For simplicity,use interrupt gates for everything"
 * https://courses.engr.illinois.edu/ece391/sp2019/secure/references/descriptors.pdf
 * idt_desc_t union data is set according to the above doc as well
 */

/* Function: idt_int;
 * Inputs: void
 * Return Value: none
 * Description: Initializes the entire idt array
 */
void idt_init(void){
	// loop index for idt array
	int i;
	// initializes the entirety of the idt array
	// values are set according to the descriptor document
	// makes only interrupt gates
	for(i = 0; i < NUM_VEC; i++){
		idt[i].seg_selector = KERNEL_CS;
		idt[i].reserved3 = 0x0;
		idt[i].reserved2 = 0x1;
		idt[i].reserved1 = 0x1;
		idt[i].size = 0x1;
		idt[i].reserved0 = 0x0;
		idt[i].present = i < 20;
		if (i == SYSCALL_IDX) {
			idt[i].dpl = 0x3;
			idt[i].present = 1;
		} else {
			// as instructed, the syscall privelege is set to 3
			idt[i].dpl = 0x0;
		}                        // all other priveleges are initialized to 0

		// sets the Intel reserved exceptions from indices 20-31
		if( (i > 19) && (i < 32) ){ SET_IDT_ENTRY(idt[i], &_db_isr ); }
	}

	SET_IDT_ENTRY(idt[0], &_de_isr );      // sets Divide by Zero exception
	SET_IDT_ENTRY(idt[1], &_db_isr );      // sets Intel Reserved exception
	SET_IDT_ENTRY(idt[2], &_nmi_isr );     // sets NMI interrupt exception
	SET_IDT_ENTRY(idt[3], &_bp_isr );      // sets Breakpoint exception
	SET_IDT_ENTRY(idt[4], &_of_isr );      // sets Overflow exception
	SET_IDT_ENTRY(idt[5], &_br_isr );      // sets Bound Range exception
	SET_IDT_ENTRY(idt[6], &_ud_isr );      // sets Invalid Opcode exception
	SET_IDT_ENTRY(idt[7], &_nm_isr );      // sets Device Not Available exception
	SET_IDT_ENTRY(idt[8], &_df_isr );      // sets Double Fault exception
	SET_IDT_ENTRY(idt[9], &_cso_isr );     // sets Coprocessor Segment Overrun exception
	SET_IDT_ENTRY(idt[10], &_ts_isr );     // sets Invalid TSS exception
	SET_IDT_ENTRY(idt[11], &_np_isr );     // sets Segment Not Present exception
	SET_IDT_ENTRY(idt[12], &_ss_isr );     // sets Stack-Segment Fault exception
	SET_IDT_ENTRY(idt[13], &_gp_isr );     // sets General Protection excpetion
	SET_IDT_ENTRY(idt[14], &_pf_isr );     // sets Page Fault exception
	SET_IDT_ENTRY(idt[15], &_db_isr );     // sets the second Intel Reserved exception
	SET_IDT_ENTRY(idt[16], &_mf_isr );     // sets x87 Floating Point Error exception
	SET_IDT_ENTRY(idt[17], &_ac_isr );     // sets Alignment Check exception
	SET_IDT_ENTRY(idt[18], &_mc_isr );     // sets Machine Check expception
	SET_IDT_ENTRY(idt[19], &_xf_isr );     // sets SIMD Floating Point exception

	/* temp set up just to check if idt[syscall] can be non null */
	SET_IDT_ENTRY(idt[SYSCALL_IDX], &_syscall_isr);
}
