#include "lib.h"
#include "idt.h"
#include "i8259.h"
#include "x86_desc.h"
#include "term.h"
#include "kb.h"

static uint8_t shift, ctrl, alt, caps;
// A simple state machine for detecting 2-byte sequences
// Other long sequences will cause undefined behaviour e.g. PrtScr & Pause break
static uint8_t scan_state = 0;

key_t keyboard_map[PRINT_KEY_NUM] = {
    R(0), R(KEY_ESC), P('1'),P('2'), P('3'),P('4'), P('5'), P('6'),
    P('7'), P('8'), P('9'),P('0'), P('-'),P('='),R(KEY_BACK),R(KEY_TAB),
    P('q'), P('w'), P('e'),P('r'), P('t'),P('y'), P('u'), P('i'),
    P('o'), P('p'), P('['),P(']'),R(KEY_ENTER), R(0), P('a'), P('s'),
    P('d'),P('f'), P('g'),P('h'), P('j'), P('k'), P('l'), P(';'),
    P('\''),P('`'), R(0), P('\\'), P('z'), P('x'),P('c'), P('v'),
    P('b'), P('n'), P('m'), P(','), P('.'), P('/'), R(0), P('*'),
    R(0), P(' '),
};

key_t shift_map[PRINT_KEY_NUM] = {
    R(0), R(KEY_ESC), P('!'),P('@'), P('#'),P('$'), P('%'), P('^'),
    P('&'), P('*'), P('('),P(')'), P('_'),P('+'),R(KEY_BACK),R(KEY_TAB),
    P('Q'), P('W'), P('E'),P('R'), P('T'),P('Y'), P('U'), P('I'),
    P('O'), P('P'), P('{'),P('}'), R(KEY_ENTER), R(0),
    P('A'), P('S'), P('D'),P('F'), P('G'),P('H'), P('J'), P('K'),
    P('L'), P(':'), P('"'),P('~'), R(0),
    P('|'), P('Z'), P('X'),P('C'), P('V'),P('B'), P('N'), P('M'),
    P('<'), P('>'), P('?'), R(0),
    P('*'), R(0),
    P(' '),
};

key_t ctrl_map[PRINT_KEY_NUM] = {
    R(0), C(KEY_ESC), C('1'),C('2'), C('3'),C('4'), C('5'), C('6'),
    C('7'), C('8'), C('9'),C('0'), C('-'),C('='),C(KEY_BACK),C(KEY_TAB),
    C('q'), C('w'), C('e'),C('r'), C('t'),C('y'), C('u'), C('i'),
    C('o'), C('p'), C('['),C(']'),C(KEY_ENTER), R(0), C('a'), C('s'),
    C('d'),C('f'), C('g'),C('h'), C('j'), C('k'), C('l'), C(';'),
    C('\''),C('`'), R(0), C('\\'), C('z'), C('x'),C('c'), C('v'),
    C('b'), C('n'), C('m'), C(','), C('.'), C('/'), R(0), C('*'),
    R(0), C(' '),
};

key_t alt_map[PRINT_KEY_NUM] = {
    R(0), A(KEY_ESC), A('1'),A('2'), A('3'),A('4'), A('5'), A('6'),
    A('7'), A('8'), A('9'),A('0'), A('-'),A('='),A(KEY_BACK),A(KEY_TAB),
    A('q'), A('w'), A('e'),A('r'), A('t'),A('y'), A('u'), A('i'),
    A('o'), A('p'), A('['),A(']'),A(KEY_ENTER), R(0), A('a'), A('s'),
    A('d'),A('f'), A('g'),A('h'), A('j'), A('k'), A('l'), A(';'),
    A('\''),A('`'), R(0), A('\\'), A('z'), A('x'),A('c'), A('v'),
    A('b'), A('n'), A('m'), A(','), A('.'), A('/'), R(0), A('*'),
    R(0), A(' '),
};

void init_kb(void) {
    cli();

	enable_irq(KB_IRQ);
	// Set interrupt handler
	SET_IDT_ENTRY(idt[KB_INT], _kb_isr);
	idt[KB_INT].present = 1;

    sti();
}

void kb_isr(void) {
    uint8_t status;
    uint8_t keycode;

    // Acknowledgment
    status = inb(0x64);
    // Lowest bit of status will be set if buffer is not empty
    if (!(status & 0x01)) {
        return;
    }
    keycode = inb(0x60);
    if (keycode == 0xE0) {
        scan_state = 1;
        return;
    }
    // Key is pressed if MSB is set
    uint8_t pressed = !(keycode & 0x80);
    key_t key;
    keycode = keycode & 0x7F;
    if (scan_state == 0) {
        switch (keycode) {
            case 0x2A:      // Left shift
            case 0x36:      // Right shift
                shift = pressed;
                break;

            case 0x1D:      // Left control
                ctrl = pressed;
                break;

            case 0x38:      // Left alt
                alt = pressed;
                break;

            case 0x3A:      // Capslock
                if (pressed) {
                    caps ^= 1;
                }
                break;

            case 0x57:      // F11
                if (pressed) {
                    term_key_handler((key_t) R(KEY_F11));
                    if (ctrl) key.modifiers |= MOD_CTRL;
                    if (shift) key.modifiers |= MOD_SHIFT;
                    if (alt) key.modifiers |= MOD_ALT;
                    term_key_handler(key);
                }
                break;

            case 0x58:      // F12
                if (pressed) {
                    term_key_handler((key_t) R(KEY_F12));
                    if (ctrl) key.modifiers |= MOD_CTRL;
                    if (shift) key.modifiers |= MOD_SHIFT;
                    if (alt) key.modifiers |= MOD_ALT;
                    term_key_handler(key);
                }
                break;

            default:        // F1 to F10
                if (keycode >= 0x3B && keycode <= 0x44 && pressed) {
                    key = (key_t) R(KEY_F1 + keycode - 0x3B);
                    if (ctrl) key.modifiers |= MOD_CTRL;
                    if (shift) key.modifiers |= MOD_SHIFT;
                    if (alt) key.modifiers |= MOD_ALT;
                    term_key_handler(key);
                    break;
                }

                if (keycode >= PRINT_KEY_NUM) { // We've got a key we can't handle
                    return;
                }

                // Handle the keys *only* when they're pressed
                if (!pressed) {
                    return;
                }
                // Emit uppercase letters when only CAPS or shift is on
                if (caps) {
                    if (!shift && keyboard_map[keycode].key >= 'a' 
                               && keyboard_map[keycode].key <= 'z') {
                        term_key_handler(shift_map[keycode]);
                    } else {
                        term_key_handler(keyboard_map[keycode]);
                    }
                    break;
                }
                if (shift) {
                    term_key_handler(shift_map[keycode]);
                    break;
                }

                if (ctrl) {
                    term_key_handler(ctrl_map[keycode]);
                    break;
                }

                if (alt) {
                    term_key_handler(alt_map[keycode]);
                    break;
                }

                term_key_handler(keyboard_map[keycode]);
        }
    } else if (scan_state == 1) {   // We've consumed a 0xE0 byte
        switch (keycode) {
            case 0x1D:      // Right control
                ctrl = pressed;
                break;

            case 0x38:      // Right alt
                alt = pressed;
                break;

            case 0x4B:
                if (pressed) {
                    term_key_handler((key_t) R(KEY_LEFT));
                }
                break;

            case 0x4D:
                if (pressed) {
                    term_key_handler((key_t) R(KEY_RIGHT));
                }
                break;
        }
        scan_state = 0;
    }
}
