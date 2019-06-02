// vim:ts=4 noet
#include "rtc.h"
#include "idt.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "task.h"
#include "syscall.h"

#define RTC_SYS_START_FREQ 2

static FILE *rtc_files[MAX_PROC_NUM] = {NULL};

file_ops_table_t rtc_file_ops_table = {
    .open = rtc_open,
    .read = rtc_read,
    .write = rtc_write,
    .close = rtc_close,
};


/* RTC driver
 * The users RTC is virtualized. Also, the maximum number of user RTC that a system
 * can keep track of is limited by `MAX_PROC_NUM`.
 *
 * Before using RTC, the system must call init_rtc() to initialize.
 *
 * Every opened RTC descriptor should be closed after used, this is essential
 * since the system's RTC frequency is determined by the maximum frequency among all
 * opened RTC descriptors, not doing this would possiblely introduce enormous overhead
 * to the system.
 */

/*
 * There are two global staitc variables which can be used as a refernece to system freq
 * `sys_freq` and `sys_freq_pow`.
 * `sys_rtc_freq` is 2 to the power of `sys_freq_pow`.
 * Be aware that always use `rtc_set_pi_freq()` to change the systems RTC freq.
 *
 * In order to vertualize the RTC, we must keep track of the user's frequency and
 * systems frequency seperately. By treating RTC is a file, we utilize the existing
 * fields in a `FILE` type varibale to store multiple infos.
 * We split FILE.inode into 3 parts, which represent `ratio`, `usr_freq`.
 * `usr_freq` is the user is frequency.
 * `ratio`    is defined as (sys_freq_pow - user_freq),
 * Be aware that `usr_freq`, and `ratio` are based on the power of 2.
 * namely, number 5 in `usr_freq` represent 2^5=32 Hz.
 *
 * File.inode (int32_t)
 * +------------------+----------------------+------------------|--------------------+
 * |  12 bit unsued   |   count ( 10bits)    |   ratio (5 bits) |  usr_freq (5 bits) |
 * +------------------+----------------------+------------------|--------------------+
 *
 * There are some functions defined below in order to operate on these data more easily
 *
 * To set the value, use:
 * 		set_rtc_ratio_field(inode, ratio)
 * 		set_rtc_freq_field(inode, freq)
 * 		set_rtc_count_field(inode, count)
 *
 * To get the value, use:
 * 		get_rtc_freq(inode)
 * 		get_rtc_ratio(inode)
 * 		set_rtc_count(inode)
 *
 *
 */

/* Design concept
 *	Abbreviation:
 *		s_freq: system RTC frequency
 *		u_freq: user RTC frequency
 * By ensuring s_freq to be the maximum number among all u_freq s,
 * the s_freq is always being a multiple of u_freq.
 *
 * Keeping the ratio between s_freq and u_freq in FILE.inode has 2 reasons.
 * 		1. trace the system frequency the last time this RTC was config.
 * 		2. store the initial value of the counter (FILE.pos)
 * We make the counter count toward 0.
 */

#define MSK_5 0x1F
#define MSK_10 0x3FF

#define RTC_FREQ_SHIFT 0
#define RTC_RATIO_SHIFT 5
#define RTC_COUNT_SHIFT 10

#define RTC_FREQ_MSK  ( MSK_5 << RTC_FREQ_SHIFT )
#define RTC_RATIO_MSK ( MSK_5 << RTC_RATIO_SHIFT )
#define RTC_COUNT_MSK ( MSK_10 << RTC_COUNT_SHIFT )

#define set_by_msk( dst, msk, src ) ( (dst) = ((dst)&(~msk)) | ((msk)&(src)) )

// macro function to use
#define set_rtc_freq_field(inode, freq) ( set_by_msk( (inode), RTC_FREQ_MSK, ((freq)<<RTC_FREQ_SHIFT)) )
#define set_rtc_ratio_field(inode, ratio) ( set_by_msk( (inode), RTC_RATIO_MSK, ((ratio)<<RTC_RATIO_SHIFT)) )
#define set_rtc_count_field(inode, count) ( set_by_msk( (inode), RTC_COUNT_MSK, ((count)<<RTC_COUNT_SHIFT)) )


#define get_rtc_freq(inode) ( ((inode) & RTC_FREQ_MSK) >> RTC_FREQ_SHIFT )
#define get_rtc_ratio(inode) ( ((inode) & RTC_RATIO_MSK) >> RTC_RATIO_SHIFT )
#define get_rtc_count(inode) ( ((inode) & RTC_COUNT_MSK) >> RTC_COUNT_SHIFT )

//#define calc_ratio(sys_freq_pow, inode)  ( (sys_freq_pow) - get_rtc_freq( (inode) ))

// Signal
// We want to send periodic signals to program every 10 second.
// Since the system RTC freq might change during execution, and the max freq is 8192 Hz.
// So we make these counter count up to 81920, and take different step under different freq.
// The equation for step is (8192/sys freq)
#define SYS_COUNTER_MAX (RTC_SYS_MAX_FREQ*10)
int32_t time_elasped[MAX_PROC_NUM] = {0};
int32_t sys_counter_step;


// represent the RTC frequency, These variables should only be set by rtc_set_pi_freq().
static int32_t sys_freq;
static int32_t sys_freq_pow;

/* init_rtc
 *	Descrption:	init system RTC.
 *
 *	Arg: none
 * 	RETURN:
 * 	none
 */
void init_rtc(void) {
	cli();
	// Set interrupt handler
	SET_IDT_ENTRY(idt[RTC_INT], _rtc_isr);
	idt[RTC_INT].present = 1;

	// Enable IRQs
	enable_irq(RTC_IRQ);
	enable_irq(SLAVE_PIC_IRQ);

	// Init routine from https://wiki.osdev.org/RTC
	// Enable interrupt
	outb(RTC_REG_B, RTC_ADDR_PORT);		// select register B, and disable NMI
	char prev = inb(RTC_DATA_PORT);		// read the current value of register B
	outb(RTC_REG_B, RTC_ADDR_PORT);		// set the index again (a read will reset the index to register D)
	outb(prev | 0x40, RTC_DATA_PORT);	// write the previous value ORed with 0x40. This turns on bit 6 of register B

	outb(RTC_FREQ_SELECT, RTC_ADDR_PORT);	// set index to register A, disable NMI
	prev = inb(RTC_DATA_PORT);				// get initial value of register A
	outb(RTC_FREQ_SELECT, RTC_ADDR_PORT);	// reset index to A
	outb((prev & 0xF0) | 3, RTC_DATA_PORT);	//write only our rate to A. Note, rate is the bottom 4 bits.

	// default system frequency: 2Hz
	sys_freq_pow=1;
	sys_freq = 1<<sys_freq_pow;
	sys_counter_step = RTC_SYS_MAX_FREQ/2;

	sti();
}

/* reset_rtc_info
 *	Descrption:	Setup the default RTC internal data sturcture based
 *		on users_freq. This func is interrupt-safe, but would 
 *		modify file->inode & file->pos.
 *	Arg: 
 *		file: pointer to file descriptor
 *		freq_pow: users frequency, NOTE that this argument is
 * 			"to the power of 2". i.e. number 5 in `freq_pow` represent 2^5=32 Hz
 * 	RETURN:
 * 	none
 */
void reset_rtc_info( FILE *file, int32_t freq_pow ){
	int32_t inode_val=0;
	set_rtc_freq_field( inode_val, freq_pow);
	set_rtc_ratio_field( inode_val, sys_freq_pow-freq_pow );
	set_rtc_count_field( inode_val, 0);
	file->inode = inode_val;
	file->pos = 1<<get_rtc_ratio(inode_val);
}

/* rtc_set_pi_freq
 *	Descrption:
 *		Set the RTC Hardware periodic interrupt frequency. Any change of the
 *		system RTC frequency should be done through this function. Also,
 *		this function would ensure the consistency between user and system RTC freq.
 *
 *	Arg: freq: must be a power of 2 and inside the range of [2,8192]
 *		isSys: if the caller is system (system can reach 8192Hz, however user can
 * 			only reach 1024hz)
 * 	RETURN:
 * 		-1 if failed
 * 		0  if sucess
 *	reference :https://github.com/torvalds/linux/blob/master/drivers/char/rtc.c
 */
int32_t rtc_set_pi_freq(int32_t freq){
	int rtc_rate_val, freq_pow;
	char prev;

	// frequency not change
	if( freq == sys_freq ){
		return 0;
	}

	//Calculate real rtc register value
	freq_pow =1;
	while (freq > (1<<freq_pow))
		freq_pow++;
	if (freq != (1<<freq_pow))
		return -1;
	// set rtc
	rtc_rate_val = 16 - freq_pow; // 8192 is the 2 to the power of 16.

	// set frequency
	cli();
	sys_freq_pow=freq_pow;
	sys_freq = 1<<sys_freq_pow;
	sys_counter_step = RTC_SYS_MAX_FREQ>>sys_freq_pow;

	outb(RTC_FREQ_SELECT, RTC_ADDR_PORT);	// set index to register A, disable NMI
	prev = inb(RTC_DATA_PORT);				// get initial value of register A
	outb(RTC_FREQ_SELECT, RTC_ADDR_PORT);	// reset index to A
	outb((prev & 0xF0) | (rtc_rate_val&0xF), RTC_DATA_PORT);        //write only our rate to A. Note, rate is the bottom 4 bits.
	sti();

	// check and update all rtc field
	int i=0;
	FILE *rtc;
	for( i=0;i<MAX_PROC_NUM;i++){
	  if( (rtc=rtc_files[i]) ){
			reset_rtc_info(rtc, get_rtc_freq(rtc->inode));
	  }
	}
	return 0;
}

/* rtc_write
 *	Descrption:	Set the RTC Hardware periodic interrupt frequency
 *
 *	Arg: freq: must be a power of 2 and inside the range of [2,8192]
 *		isSys: if the caller is system (system can reach 8192Hz, however user can
 * 			only reach 1024hz)
 * 	RETURN:
 * 		-1 if failed
 * 		0  if sucess
  */
int32_t rtc_write(const int8_t *buf, uint32_t length, FILE *file){
	if (length < 4 || !buf) {
		return -1;
	}

	uint32_t freq = *(int32_t *) buf;
	if (freq > RTC_USER_MAX_FREQ) {
		return -1;
	}

	int freq_pow=1;
	while (freq > (1<<freq_pow))
		freq_pow++;
	if (freq != (1<<freq_pow))
		return -1;

	if(freq > sys_freq ){
		set_rtc_freq_field(file->inode, freq_pow);
		
		// set system frequency, also calc all inode structure
		rtc_set_pi_freq(freq);
	}else{
		reset_rtc_info(file,freq_pow);
	}
	return 0;
}

/* rtc_isr
 *	Descrption:	system RTC interrupt handler,
 *
 *	Arg: none
 * 	RETURN: none
  */
void rtc_isr(void) {
	outb(0x0C, RTC_ADDR_PORT);
	(void) inb(RTC_DATA_PORT);
	uint8_t i;
	int count ;
	for (i = 0; i < MAX_PROC_NUM; i ++) {
		time_elasped[i] += sys_counter_step;
		if (time_elasped[i] >= SYS_COUNTER_MAX){
			time_elasped[i] = 0;
			PCB_t *task_pcb = get_cur_pcb();
			task_pcb->signals |= SIG_FLAG(SIG_ALARM);
		}
		FILE *cur_file = rtc_files[i];
		if (cur_file) {
			cur_file->pos -= 1;
			if (cur_file->pos <= 0) {
				cur_file->pos = 1<<get_rtc_ratio(cur_file->inode);
				count = get_rtc_count(cur_file->inode)+1;
				if(count >32){
					// The system is overwhelmed in this situation
				 	count = 32;
				}
				set_rtc_count_field(cur_file->inode, count );
			}
		}
	}
}

/* rtc_read
 *	Descrption:	a user blocking function intended to wait for the next RTC interrupt.
 *	Args:
 *		buf: (not used) Use NULL in this argument
 *		length: (not used) Use 0 in this argument
 *		file: RTC file descriptor
 * 	RETURN: none
  */
int32_t rtc_read(int8_t* buf, uint32_t length, FILE *file){
	int count;
	sti();
	while( (count=get_rtc_count(file->inode)) == 0 ){
		asm volatile ("hlt");
	}
	cli();
	if( count >=1 ){
		count -= 1;
		set_rtc_count_field(file->inode, count);
	}
	return 0;
}

/* rtc_open
 *	Descrption:	open a rtc descriptor for a process.
 *	Args:
 *		filename: (not used) use "" in this argument.
 *		file: pointer to the RTC file descriptor
 * 	RETURN: 0 if success
		halt the system if too many RTC is opend in this system.
  */
int32_t rtc_open(const int8_t *filename, FILE *file){
	int freq_pow;
	int32_t usr_freq;

	//Calculate real rtc register value
	freq_pow =1;
	usr_freq = RTC_USER_DEF_FREQ;
	while (usr_freq > (1<<freq_pow))
		freq_pow++;
	if ( usr_freq != (1<<freq_pow))
		return -1;

	reset_rtc_info(file,freq_pow);
	file->file_ops = &rtc_file_ops_table;
	file->flags.type = TASK_FILE_RTC;

	uint8_t i;
	for (i = 0; i < MAX_PROC_NUM; i ++) {
		if (!rtc_files[i]) {
			rtc_files[i] = file;
			time_elasped[i] = 0;
			return 0;
		}
	}

	while (1) { asm volatile ("hlt;"); } 	// UNREACHABLE!!!
}

/* rtc_close
 *	Descrption:	close a rtc descriptor for a process.
 *	Args:
 *		file: pointer to the RTC file descriptor
 * 	RETURN:
		0 if success
  */
int32_t rtc_close(FILE *file){
	uint8_t i;
	uint32_t max_i = -1;
	uint32_t max_freq = -1;
	for (i = 0; i < MAX_PROC_NUM; i ++) {
		if (rtc_files[i] == file) {
			rtc_files[i] = NULL;
		}
	}
	uint32_t tmp_freq;
	for( i=0 ; i<MAX_PROC_NUM ; i++){
		if( rtc_files[i] && (tmp_freq=get_rtc_freq(rtc_files[i]->inode))>max_freq){
			max_freq = tmp_freq;
			max_i = i;
		}
	}
	if(max_i!=-1){
		return rtc_set_pi_freq( max_freq);
	}
	return 0;
}
