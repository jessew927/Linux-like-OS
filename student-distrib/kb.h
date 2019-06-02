#ifndef _KB_H_
#define _KB_H_

#include "types.h"

// Distinguish between printable and non-printable keys
typedef enum {
    KEY_RAW = 0,
    KEY_PRINT,
} key_type_t;

// Non-printable keys to be handled
typedef enum {
    KEY_TAB = 0x80,
    KEY_ESC,
    KEY_BACK,
    KEY_ENTER,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_INSERT,
    KEY_DEL,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_PRTSCR,
} raw_key_t;

// Type for uniform key handling
typedef struct {
    key_type_t t;
    uint8_t modifiers;
    uint8_t key;
} key_t;

// Arbitrary bit masks
#define MOD_CTRL    0x80
#define MOD_SHIFT   0x40
#define MOD_ALT     0x20

#define P(c) {KEY_PRINT, 0, c}
#define R(c) {KEY_RAW, 0, c}
#define C(c) {KEY_RAW, MOD_CTRL, c}
#define S(c) {KEY_RAW, MOD_SHIFT, c}
#define A(c) {KEY_RAW, MOD_ALT, c}

// Number of printble keys
#define PRINT_KEY_NUM 0x3A

void init_kb(void);
void kb_isr(void);

#endif
