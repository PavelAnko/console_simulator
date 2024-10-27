#ifndef UTILS_H
#define UTILS_H
#include "../kernel/kernel.h"
#include "../drivers/keyboard/keyboard.h"
#include "../drivers/timer/timer.h"
#include "../drivers/serial_port/serial_port.h"

#define MAX_ROWS 25
#define MAX_COLS 80

extern char *framebuffer;
extern unsigned short cursor_pointer;
extern int TIMER_TICKES;
extern int KEY_PRESSED;
extern char *framebuff;
extern bool STARTED;
extern bool edit_mode;
extern unsigned char file_slot_indx;
extern int bcgr_color;
extern char *new_line;
extern unsigned short cnsl_state_pntr;
extern unsigned char cnsl_state_chrs[25 * 80];
extern char SAVE[MAX_ROWS * MAX_COLS * 2];
extern unsigned char files_names[10][75]; 
extern unsigned char files_slots[10][1];           
extern unsigned char files_content[10][2000];     
extern unsigned short files_last_chr_indx[10][1];
extern unsigned char files_count; 
extern unsigned char line_st_ofst;
extern unsigned char *line_char;

extern void put_cursor(unsigned short pos);
extern void print_char (int row, int colum, char c); 
extern void new_line_handle (unsigned short *current_row, unsigned short *current_col, bool *NEW_LINE);
extern void clean_screen();
extern void print(char *msg, unsigned char clr_bckg, unsigned char clr_fnt);
extern void cmnd_clear();
extern void cmnd_help();
extern void cmnd_sleep();
extern void cmnd_list();
extern void print_info_cmd(char *msg);
extern void cmnd_rename(unsigned short *pntr_aftr_cmnd);
extern void cmnd_edit(unsigned short *pntr_aftr_cmnd);
extern void cmnd_read(unsigned short *pntr_aftr_cmnd);
extern void cmnd_delete(unsigned short *pntr_aftr_cmnd);
extern void cmnd_create(unsigned short *pntr_aftr_cmnd);
extern char search_command(unsigned short *position_text);
extern int rand();
extern void start_animation();
extern void create_new_folder_un();
#endif