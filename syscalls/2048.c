#include <stdbool.h>
#include "printf.h"

bool check_end (int tile[16]);
bool check_if_session ();
char get_key ();
int move_tile (int tile[16], char direction);
struct termios set_non_canonical ();
void load_session (int cur_tile[16], int cur_score, char *session);
void save_session (int cur_tile[16], int cur_score);
void map_key (char key);
void place_tile (int tile[16]);
void print_border ();
void print_info (int status);
void print_load_session ();
void print_status (int score);
void print_tile (int num, int tile_num);

char getchar() {
    char c;
    asm volatile (
        "mov $3, %%eax;"
        "mov $0, %%ebx;"
        "mov %0, %%ecx;"
        "mov $1, %%edx;"
        "int $0x80;"
        :
        : "g" (&c)
        : "%eax", "%ebx", "%ecx", "%edx"
    );
    return c;
}

unsigned int random() {
    static unsigned int lfsr = 0xC0FFEE;
    unsigned int bit;

    /* taps: 16 14 13 11; feedback polynomial: x^16 + x^14 + x^13 + x^11 + 1 */
    bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) /* & 1u */;
    lfsr = (lfsr >> 1) | (bit << 15);

    return lfsr;
}

void print_border () {
    int c, r;
    char horizontal[44] = "-------------------------------------------";
    //Print horizontally
    for (r=1;r<4;r++)
        printf ("\e[%d;%dp%s", r * 6, 1, horizontal);

    //Print vertically
    for (c=1;c<5;c++)
        for (r=1;r<24;r++)
            printf ("\e[%d;%dp|", r, 11 * c);

    //Print dot
    for (c=1;c<5;c++)
        for (r=1;r<4;r++)
            printf ("\e[%d;%dp%c", r * 6, c * 11, '+');
}

void print_tile (int num, int tile_num) {
    char *color, *before, *after;
    int row, col;
    int p_y, p_x;

    switch (num) {
        case 2:
            color = "\e[1b";
            before = "    ";
            after = "     ";
            break;
        case 4:
            color = "\e[2b";
            before = "    ";
            after = "     ";
            break;
        case 8:
            color = "\e[3b";
            before = "    ";
            after = "     ";
            break;
        case 16:
            color = "\e[4b";
            before = "    ";
            after = "    ";
            break;
        case 32:
            color = "\e[5b";
            before = "    ";
            after = "    ";
            break;
        case 64:
            color = "\e[6b";
            before = "    ";
            after = "    ";
            break;
        case 128:
            color = "\e[7b";
            before = "   ";
            after = "    ";
            break;
        case 256:
            color = "\e[8b";
            before = "   ";
            after = "    ";
            break;
        case 512:
            color = "\e[9b";
            before = "   ";
            after = "    ";
            break;
        case 1024:
            color = "\e[10b";
            before = "   ";
            after = "   ";
            break;
        case 2048:
            color = "\e[11b";
            before = "   ";
            after = "   ";
            break;
        case 4096:
            color = "\e[12b";
            before = "   ";
            after = "   ";
            break;
        default:
            break;
    }

    row = (int) (tile_num/4+1);
    col = tile_num % 4 + 1;
    p_y = (row - 1) * 6 + 1;
    p_x = (col - 1) * 11 + 1;
    if (num == 0) {
        printf ("\e[f\e[b");
        printf ("\e[%d;%dp          ", p_y, p_x);
        printf ("\e[%d;%dp          ", p_y+1, p_x);
        printf ("\e[%d;%dp          ", p_y+2, p_x);
        printf ("\e[%d;%dp          ", p_y+3, p_x);
        printf ("\e[%d;%dp          ", p_y+4, p_x);
        printf ("\e[f\e[b");
    } else {
        printf ("%s", color);
        printf ("\e[%d;%dp          ", p_y, p_x);
        printf ("\e[%d;%dp          ", p_y+1, p_x);
        printf ("\e[%d;%dp%s%d%s", p_y+2, p_x, before, num, after);
        printf ("\e[%d;%dp          ", p_y+3, p_x);
        printf ("\e[%d;%dp          ", p_y+4, p_x);
        printf ("\e[f\e[b");
    }
}

char get_key () {
    char signal;
    char key;
    signal = getchar ();
    switch (signal) {
        case 'w': case 'W': key = 'U'; break;
        case 's': case 'S': key = 'D'; break;
        case 'd': case 'D': key = 'R'; break;
        case 'a': case 'A': key = 'L'; break;
        case 'r': case 'R': key = 'r'; break;
        case 'q': case 'Q': key = 'q'; break;
        case 'i': case 'I':
        case 'h': case 'H': key = 'h'; break;
        default:            key = 'x'; break;
    }
    return key;
}

int move_tile (int tile[16], char direction) {
    int i, j;
    int delta_score = 0;

    switch (direction) {
        case 'U':
            for (j=0;j<=5;j++) {
                if (j!=4) {
                    for (i=0;i<12;i++) {
                        if (tile[i] == 0) {
                            tile[i] = tile[i+4];
                            tile[i+4] = 0;
                        }
                    }
                } else {
                    for (i=0;i<12;i++) {
                        if (tile[i] == tile[i+4]) {
                            tile[i] = 2 * tile[i];
                            tile[i+4] = 0;
                            delta_score += tile[i];
                        }
                    }
                }
            }
            break;
        case 'D':
            for (j=0;j<=5;j++) {
                if (j!=4) {
                    for (i=11;i>=0;i--) {
                        if (tile[i+4] == 0) {
                            tile[i+4] = tile[i];
                            tile[i] = 0;
                        }
                    }
                } else {
                    for (i=11;i>=0;i--) {
                        if (tile[i] == tile[i+4]) {
                            tile[i+4] = 2 * tile[i+4];
                            tile[i] = 0;
                            delta_score += tile[i+4];
                        }
                    }
                }
            }
            break;
        case 'R':
            for (j=0;j<=5;j++) {
                if (j!=4) {
                    for (i=14;i>=0;i--) {
                        if (i == 3 || i == 7 || i == 11) {
                            continue;
                        }
                        if (tile[i+1] == 0) {
                            tile[i+1] = tile[i];
                            tile[i] = 0;
                        }
                    }
                } else {
                    for (i=14;i>=0;i--) {
                        if (i == 3 || i == 7 || i == 11) {
                            continue;
                        }
                        if (tile[i] == tile[i+1]) {
                            tile[i+1] = 2 * tile[i+1];
                            tile[i] = 0;
                            delta_score += tile[i+1];
                        }
                    }
                }
            }
            break;
        case 'L':
            for (j=0;j<=5;j++) {
                if (j!=4) {
                    for (i=0;i<=14;i++) {
                        if (i == 3 || i == 7 || i == 11)
                            continue;
                        if (tile[i] == 0) {
                            tile[i] = tile[i+1];
                            tile[i+1] = 0;
                        }
                    }
                } else {
                    for (i=0;i<=14;i++) {
                        if (i == 3 || i == 7 || i == 11)
                            continue;
                        if (tile[i] == tile[i+1]) {
                            tile[i] = 2 * tile[i];
                            tile[i+1] = 0;
                            delta_score += tile[i];
                        }
                    }
                }
            }
            break;
    }
    return delta_score;
}

void place_tile (int tile[16]) {
    int i;
    int empty_loc;
    int tile_loc;
    int tile_num;
    int empty_tile[16];
    int empty_num=0;

    /* srandom (time (NULL)); */
    for (i=0;i<16;i++) {
        if (0 == tile[i]) {
            empty_tile[empty_num] = i;
            ++empty_num;
        }
    }

    if (empty_num > 0) {
        tile_num = (random () % 10) ? 2 : 4;
        empty_loc = random () % (empty_num);
        tile_loc = empty_tile[empty_loc];
        tile[tile_loc] = tile_num;
    }
}

void print_status (int score) {
    printf ("\e[24;1p");
    printf ("\e[0;f\e[15b");
    printf ("%79s\r%70d\r%60s\r%s", "          ", score, "YOUR SCORE: ", "-- THE 2048 GAME --");
    printf ("\e[24;80p\e[f\e[b");
}

//INFO
void print_info (int status) {
    printf("\e[f\e[b");
    switch (status) {
        case 0: //normal
            printf ("\e[3;45p%s", "    Use your arrow key to play      ");
            printf ("\e[4;45p%s", "                                    ");
            printf ("\e[5;45p%s", "               _____                ");
            printf ("\e[6;45p%s", "              |     |               ");
            printf ("\e[7;45p%s", "              |  ^  |               ");
            printf ("\e[8;45p%s", "              |  |  |               ");
            printf ("\e[9;45p%s", "        -------------------         ");
            printf("\e[10;45p%s", "        |     |     |     |         ");
            printf("\e[11;45p%s", "        |  <- |  |  | ->  |         ");
            printf("\e[12;45p%s", "        |     |  v  |     |         ");
            printf("\e[13;45p%s", "        -------------------         ");
            printf("\e[14;45p%s", "       Or W, S, A, D instead        ");
            break;
        case 1: //win
            printf ("\e[3;45p%s", "                                    ");
            printf ("\e[4;45p%s", "                                    ");
            printf ("\e[5;45p%s", "                                    ");
            printf ("\e[6;45p%s", "                                    ");
            printf ("\e[7;45p%s", "                                    ");
            printf ("\e[8;45p%s", "                                    ");
            printf ("\e[9;45p%s", "                                    ");
            printf("\e[10;45p%s", "                                    ");
            printf("\e[11;45p%s", "                                    ");
            printf("\e[4f");;
            printf("\e[12;45p%s", "              HURRAY!!!             ");
            printf("\e[5f");;
            printf("\e[13;45p%s", "         You reached 2048!!         ");
            printf("\e[f\e[b");
            printf("\e[14;45p%s", "          Move to continue          ");
            break;
        case 2: //lose
            printf ("\e[3;45p%s", "                                    ");
            printf ("\e[4;45p%s", "                                    ");
            printf ("\e[5;45p%s", "                                    ");
            printf ("\e[6;45p%s", "                                    ");
            printf ("\e[7;45p%s", "                                    ");
            printf ("\e[8;45p%s", "                                    ");
            printf ("\e[9;45p%s", "                                    ");
            printf("\e[10;45p%s", "                                    ");
            printf("\e[11;45p%s", "                                    ");
            printf("\e[12;45p%s", "                                    ");
            printf("\e[1f");;
            printf("\e[13;45p%s", "               OH NO!!              ");
            printf("\e[f\e[b");
            printf("\e[14;45p%s", "             You lose!!!            ");
            break;
        default:
            break;
    }
    printf ("\e[20;45p%s", "        Press 'r' to retry");
    printf ("\e[21;45p%s", "              'q' to quit");
}

bool check_end (int tile[16]) {
    int i;
    int tile_bak[16];
    bool changed = false;
    for (i=0;i<16;i++)
        if (tile[i] == 0)
            return false;

    for (i=0;i<16;i++)
        tile_bak[i] = tile[i];

    move_tile (tile, 'U');
    move_tile (tile, 'D');
    move_tile (tile, 'R');
    move_tile (tile, 'L');
    for (i=0;i<16;i++)
        if (tile_bak[i] != tile[i])
            changed = true;

    if (changed) {
        for (i=0;i<16;i++)
            tile[i] = tile_bak[i];
        return false;
    }
    return true;
}

int main () {
    char key;
    bool changed = false;
    bool reached = false;
    int i;
    int turn;
    int score = 0;
    int tile[16];
    int tile_bak[16];

    printf ("\e[e\e[c");    // Store cursor position; Clear screen
    print_border ();
    printf("\e[1s\e[2s");   // Set to canonical mode and no echo
start:
    //tile initialization
    for (i=0;i<16;i++)
        tile[i] = 0;

    place_tile (tile);
    place_tile (tile);

    for (i=0;i<16;i++)
        print_tile (tile[i], i);

    score = 0;
    print_info (0);
    print_status (score);
    //main loop
    for (turn=1;!check_end (tile);turn++) {

        //backup
        for (i=0;i<16;i++)
            tile_bak[i] = tile[i];

        //check 2048
        if (!reached) {
            for (i=0;i<16;i++) {
                if (tile[i] == 2048) {
                    print_info (1);
                    reached = true;
                }
            }
        } else
            print_info (0);

        //Get key
        key = get_key ();
        switch (key) {
            case 'U': case 'D': case 'R': case 'L':
                score += move_tile (tile, key);
                break;
            case 'q':
                goto end;
                break;
            case 'r':
                goto start;
                break;
            case 'h':
                break;
            default:
                break;
        }

        changed = false;
        for (i=0;i<16;i++)
            if (tile[i] != tile_bak[i])
                changed = true;

        if (changed) {
            place_tile (tile);

            for (i=0;i<16;i++) {
                print_tile (tile[i], i);
            }

            print_status (score);
        }

    }

    print_info (2);
    while (true) {
        switch (get_key ()) {
            case 'q':
                goto end;
            case 'r':
                goto start;
            default:
                continue;
        }
    }

end:
    //exit
    printf("\e[2S\e[1S");   // Reset terminal
    printf ("\e[c\e[r\n");
    return 0;
}
