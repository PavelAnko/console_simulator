#ifndef UTILS_H
#define UTILS_H
#include "../kernel/kernel.h"
#include "../drivers/keyboard/keyboard.h"
#include "../drivers/timer/timer.h"
#include "../drivers/serial_port/serial_port.h"

extern void put_cursor(unsigned short pos);
extern void print_char (int row, int colum, char c); 
// extern void move_to_new_line(unsigned short *current_row, unsigned short *current_col, unsigned short max_rows, bool *NEW_LINE);
extern void new_line_handle (unsigned short *current_row, unsigned short *current_col, bool *NEW_LINE);
// extern void check_command(unsigned short *current_row, unsigned short *current_col, const char *input, bool *ONE_VOIDE);
extern void clean_screen();
extern void print(char *msg, unsigned char clr_bckg, unsigned char clr_fnt);
extern void cmnd_clear();
extern void cmnd_help();
extern void cmnd_sleep();
extern char search_command(unsigned short *position_text);

#endif