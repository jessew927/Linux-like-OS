/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

#define ALL_MASKED    0xFF
#define TO_UNMASK     0xFE
#define TO_MASK       0x01
#define SLAVE_IRQ_OFF    8

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void i8259_init(void) {
    /* PIC initialization */
    master_mask = ALL_MASKED;
    slave_mask = ALL_MASKED;

    /* Mask all the interrupts */
    outb(ALL_MASKED, MASTER_8259_DATA);
    outb(ALL_MASKED, SLAVE_8259_DATA);

    /* ICW1 */
    outb(ICW1, MASTER_8259_PORT);
    outb(ICW1, SLAVE_8259_PORT);

    /*ICW2*/
    outb(ICW2_MASTER, MASTER_8259_DATA);
    outb(ICW2_SLAVE, SLAVE_8259_DATA);

    /*ICW3*/
    outb(ICW3_MASTER, MASTER_8259_DATA);
    outb(ICW3_SLAVE, SLAVE_8259_DATA);

    /* ICW4 */
    outb(ICW4, MASTER_8259_DATA);
    outb(ICW4, SLAVE_8259_DATA);

    /* Remask all the interrupts */
    outb(ALL_MASKED, MASTER_8259_DATA);
    outb(ALL_MASKED, SLAVE_8259_DATA);
}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
    int i, j;
    uint8_t enable = TO_UNMASK;
    /* check to see if irq_num is within valid range of 0-15*/
    if( (irq_num < 0) || (irq_num > 15) ){ return; }
    /* checks to see if the intr we want to enable is on the master or slave */
    if(irq_num < SLAVE_IRQ_OFF){
      for(i = 0; i < irq_num; i++)
        enable = (enable << 1) + 1;         // shift the 0 left by 1 without having other zeroes
      /* Update master_mask to reflect currently enable interrupts */
      master_mask = master_mask & enable;
      outb(master_mask, MASTER_8259_DATA);
    }
    else{
      for(j = SLAVE_IRQ_OFF; j < irq_num; j++)
        enable = (enable << 1) + 1;       // shift the 0 left by 1 without having other zeroes
      /* Update master_mask and slave_mask to reflect currently enable interrupts */
      slave_mask = slave_mask & enable;
      master_mask = master_mask & ~(ICW3_SLAVE);
      outb(slave_mask, SLAVE_8259_DATA);
      outb(master_mask, MASTER_8259_DATA);
    }
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
    int i, j;
    uint8_t disable = TO_MASK;
    /* check to see if irq_num is within valid range of 0-15*/
    if( (irq_num < 0) || (irq_num > 15) ){ return; }
    /* checks to see if the intr we want to disable is on the master or slave */
    if(irq_num < SLAVE_IRQ_OFF){
      for(i = 0; i < irq_num; i++)
        disable = disable << 1;
      /* Update master_mask to reflect currently enable interrupts */
      master_mask = master_mask | disable;
      outb(master_mask, MASTER_8259_DATA);
    }
    else{
      for(j = SLAVE_IRQ_OFF; j < irq_num; j++)
        disable = disable << 1;
      /* Update master_mask  and slave mask to reflect currently enable interrupts */
      slave_mask = slave_mask | disable;
      if(!slave_mask)
        master_mask = master_mask | ICW3_SLAVE;
      outb(slave_mask, SLAVE_8259_DATA);
      outb(master_mask, MASTER_8259_DATA);
    }
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {

    /* check to see if irq_num is within valid range of 0-15*/
    if( (irq_num < 0) || (irq_num > 15) ){ return; }
    /* checks to see if irq_num is on the master or slave */
    if(irq_num < SLAVE_IRQ_OFF)
      outb(irq_num | EOI, MASTER_8259_PORT);
    else{
      outb( (irq_num - SLAVE_IRQ_OFF) | EOI, SLAVE_8259_PORT);
      outb( ICW3_SLAVE | EOI, MASTER_8259_PORT );
    }
}
