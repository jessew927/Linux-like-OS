#ifndef _TERM_H_
#define _TERM_H_

#include "types.h"
#include "kb.h"
#include "task.h"

#define TERM_NUM 3
#define TERM_BUF_SIZE 127
#define TERM_BUF_SIZE_W_NL 128

#define VID_MEM_SIZE (2 * 80 * 25)

typedef enum {
    IDLE,
    A_ESCAPE,
    A_BRACKET,
    ARG_PARSE,
    PARTIAL_FINISH,
} esc_state_t;

typedef struct {
    char term_buf[TERM_BUF_SIZE_W_NL];
    uint8_t term_buf_count;
    uint8_t term_read_done;
    uint8_t term_curpos;
    int8_t cur_x, cur_y;
    int8_t cur_x_store, cur_y_store;
    uint8_t attr;
    uint8_t reading;

    uint8_t *video_mem;
    uint8_t* video_buffer;
    uint8_t term_noecho : 1;
    uint8_t term_canon : 1;
    uint8_t cur_pid;

    esc_state_t state;
    // Buffer for each argument
    int8_t buf[4];
    uint8_t buf_len;
    // All the arguments
    uint8_t args[4];
    uint8_t arg_len;
} term_t;

file_ops_table_t stdin_file_ops_table, stdout_file_ops_table;
term_t terms[TERM_NUM];
uint8_t cur_term_ind;
term_t *cur_term;

void term_key_handler(key_t key);
void init_term();
int32_t term_write(const int8_t* buf, uint32_t nbytes, FILE *file);
int32_t term_read(int8_t* buf, uint32_t nbytes, FILE *file);
int32_t term_open(const int8_t *filename, FILE *file);
int32_t term_close();

int32_t printf(term_t *cur_term, int8_t *format, ...);
void putc(uint8_t c, term_t *cur_term);
int32_t puts(int8_t *s, term_t *cur_term);
void scroll(term_t *cur_term);
void clear(term_t *cur_term);
void setpos(int x, int y, term_t *cur_term);
void setattr(uint8_t _attr, term_t *cur_term);
uint8_t getattr(term_t *cur_term);
int getposx(term_t *cur_term);
int getposy(term_t *cur_term);
void back(term_t *cur_term);
void forward(term_t *cur_term);

#endif
