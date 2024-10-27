#include "kernel/kernel.h"
#include "drivers/keyboard/keyboard.h"
#include "drivers/timer/timer.h"
#include "drivers/serial_port/serial_port.h"
#include "utils/utils.h"

void exception_handler(u32 interrupt, u32 error, char *message) {
    serial_log(LOG_ERROR, message);
}

void init_kernel() {
    init_gdt();
    init_idt();
    init_exception_handlers();
    init_interrupt_handlers();
    register_timer_interrupt_handler();
    register_keyboard_interrupt_handler();
    configure_default_serial_port();
    set_exception_handler(exception_handler);
    enable_interrupts();
}


void put_cursor(unsigned short pos) {
    out(0x3D4, 14);
    out(0x3D5, ((pos >> 8) & 0x00FF));
    out(0x3D4, 15);
    out(0x3D5, pos & 0x00FF); 
}

_Noreturn void halt_loop() {
    while (1) { halt(); }
}

// -------------- КОМАНДИ --------------

void (*cmnds[])(unsigned short *) =   
{
    cmnd_help,
    cmnd_clear,
    cmnd_sleep,
    cmnd_list,
    cmnd_create,
    cmnd_edit,
    cmnd_rename,
    cmnd_read,
    cmnd_delete,
};

// -------------- КОЛЬОРИ --------------

#define BLACK_COLOR 0x0
#define GREEN_COLOR 0xa
#define YELLOW_COLOR 0xe
#define RED_COLOR 0xc
#define WHITE_COLOR 0xf
int bcgr_color = BLACK_COLOR;  // 0 — чорний колір

// -------------- ОБМЕЖЕННЯ --------------

#define MAX_ROWS 25
#define MAX_COLS 80
#define MAX_COMMAND_LENGTH 100
#define MAX_DELAY 1000                // Максимальна затримка перед анімацією

// -------------- ОБМЕЖЕННЯ ДЛЯ ФАЙЛІВ ТА РОБОТА З НИМИ --------------

bool edit_mode = false;   
unsigned char file_slot_indx = 0;

unsigned char files_names[10][75]; 
unsigned char files_slots[10][1];           // 0 - доступний, 1 - зайнятий
unsigned char files_content[10][2000];      // Масив для зберігання вмісту файлів, максимум 10 файлів, кожен файл розміром до 2000 байт
unsigned short files_last_chr_indx[10][1];  // Поточний індекс останнього символу у файлі
unsigned char files_count = 10;

// -------------- РОБОТА В КОНСОЛІ --------------

char *framebuffer = (char *) 0xb8000; //Покажчик на буфер консолі
unsigned char *line_char = " $ ";
unsigned short cursor_pointer = 0;
unsigned char line_st_ofst = 3;  //Обмеження позиції курсора

// -------------- АНІМАЦІЯ --------------

char *framebuff = (char *) 0xb8000;
bool STARTED = false;
unsigned short current_row = 0, current_col = 0;
char SAVE[MAX_ROWS * MAX_COLS * 2];    // Для зберігання тексту
int TIMER_TICKES = 0;
int KEY_PRESSED = 0;
char *new_line = "\n";



void key_handler(struct keyboard_event event) {
    if (event.key_character && event.type == EVENT_KEY_PRESSED) {
         
        KEY_PRESSED = TIMER_TICKES;        // Оновлюємо таймер натискання клавіші
        if (STARTED) {
            return;
        }

        if (edit_mode){
            if (event.key == KEY_BACKSPACE)
            {
                if (cursor_pointer > 0) 
                {
                    files_last_chr_indx[file_slot_indx][0]--;
                    files_content[file_slot_indx][files_last_chr_indx[file_slot_indx][0]] == BLACK_COLOR;
                    if (files_content[file_slot_indx][files_last_chr_indx[file_slot_indx][0]] == '\n')
                    {
                        cursor_pointer--; 
                        while (*(framebuffer + (cursor_pointer - 1) * 2) == BLACK_COLOR && cursor_pointer % 80 != 0)
                        {
                            cursor_pointer--;
                        }
                    }
                    else 
                    {
                        cursor_pointer--;
                    }

                    *(framebuffer + cursor_pointer * 2) = BLACK_COLOR;
                    put_cursor(cursor_pointer);
                }
            }
            else if (event.key == KEY_ENTER)
            {
                if (cursor_pointer / 80 < 23)
                {
                    files_content[file_slot_indx][files_last_chr_indx[file_slot_indx][0]] = event.key_character;
                    print(&event.key_character, bcgr_color, YELLOW_COLOR);
                    files_last_chr_indx[file_slot_indx][0]++;
                }
            }
            else if (event.key == KEY_TAB)
            {
                files_content[file_slot_indx][files_last_chr_indx[file_slot_indx][0]] = '\0';
                edit_mode = false;
                clean_screen();
                recover_console_state();
                file_slot_indx = 0;
                char *new_line = "\n";
                print(new_line, BLACK_COLOR, BLACK_COLOR);
                print(line_char, BLACK_COLOR, GREEN_COLOR);
            }
            else
            {
                if (cursor_pointer % 80 < 79)
                {
                    files_content[file_slot_indx][files_last_chr_indx[file_slot_indx][0]] = event.key_character;
                    print(&event.key_character, bcgr_color, YELLOW_COLOR);
                    files_last_chr_indx[file_slot_indx][0]++;
                }
            }
        }
        
        else
        {
            if (event.key == KEY_BACKSPACE)
            {
                if (cursor_pointer % 80 > line_st_ofst)   // якщо було написано щось, де
                {
                    cursor_pointer -= 1;
                    *(framebuffer + cursor_pointer * 2) = bcgr_color;   //при зміщені очищає попередній символ
                    put_cursor(cursor_pointer);
                }
            }
            else if (event.key == KEY_ENTER)
            {
                char cmnd_nmbr = 0;
                if (cursor_pointer % 80 > line_st_ofst)   // якщо було написано щось
                {
                    unsigned short position_text = cursor_pointer - cursor_pointer % 80 + line_st_ofst;  // знаходження позиції початкового тексту в поточному рядку (85-(85%80)+3  -> 85-5+3)

                    if (*(framebuffer + position_text * 2) != bcgr_color)
                    {
                        cmnd_nmbr = search_command(&position_text);

                        if (cmnd_nmbr > -1)
                        {                            
                            cmnds[cmnd_nmbr](&position_text);
                        }
                        else if (cmnd_nmbr == -1)
                        {
                            char *msg_no_cmnd = "Command not found\n";
                            print_info_cmd(msg_no_cmnd);
                        }

                    }
                }
                else
                    print(new_line, bcgr_color, BLACK_COLOR);

                if (cmnd_nmbr != 5 && cmnd_nmbr != 2)
                {
                    print(line_char, bcgr_color, GREEN_COLOR);
                }
            }
            
            else
            {
                if (cursor_pointer % 80 < 79)
                {
                    print(&event.key_character, bcgr_color, WHITE_COLOR);
                }
            }
        }
    }
}

void timer_tick_handler() {
    TIMER_TICKES++;
    if (TIMER_TICKES - KEY_PRESSED > MAX_DELAY) {
        save_console_txt();

        if (TIMER_TICKES % 5 == 0) {
            start_animation();
        }
        
    } else {
        if (STARTED) {         // Якщо анімація завершилася, відновлюємо вміст екрану
            for (int i = 0; i < MAX_ROWS * MAX_COLS * 2; i += 2) {
                framebuff[i] = SAVE[i];       // Відновлюємо символ
                framebuff[i + 1] = SAVE[i + 1]; // Відновлюємо колір
            }
            STARTED = false; // Скидаємо анімацію
            TIMER_TICKES = 0;
            KEY_PRESSED = 0;
        }
    }
}

void kernel_entry() {
    init_kernel();
    keyboard_set_handler(key_handler);
    timer_set_handler(timer_tick_handler);
    clean_screen();
    char *ver_msg = "ChristOS - v 0.0.1\n";
    print(ver_msg, BLACK_COLOR, GREEN_COLOR);
    print(line_char, BLACK_COLOR, GREEN_COLOR); 
    halt_loop(); 
}

