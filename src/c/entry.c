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
    // cmnd_list,
    // cmnd_create,
    // cmnd_edit,
    // cmnd_read,
    // cmnd_delete,
    // flip_cmnd
};

// -------------- КОЛЬОРИ --------------

#define BLACK_COLOR 0x0
#define GREEN_COLOR 0xa
#define YELLOW_COLOR 0xe
#define RED_COLOR 0xc
#define WHITE_COLOR 0xf
#define PINK_COLOR 0xd

int bcgr_color = BLACK_COLOR;  // 0 — чорний колір

// -------------- ОБМЕЖЕННЯ --------------

#define MAX_ROWS 25
#define MAX_COLS 80
#define MAX_COMMAND_LENGTH 100
#define MAX_DELAY 150                // Максимальна затримка перед анімацією

// -------------- ОБМЕЖЕННЯ ДЛЯ ФАЙЛІВ ТА РОБОТА З НИМИ --------------

bool edit_mode = false;   //перемикачем між режимами редагування файлу і введення команд
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

bool STARTED = false;

unsigned short current_row = 0, current_col = 0;
char SAVE[MAX_ROWS * MAX_COLS * 2];    // Для зберігання тексту
int TIMER_TICKES = 0;
int KEY_PRESSED = 0;




void key_handler(struct keyboard_event event) {
    if (event.key_character && event.type == EVENT_KEY_PRESSED) {
         
        KEY_PRESSED = TIMER_TICKES;        // Оновлюємо таймер натискання клавіші
        if (edit_mode){
            if (event.key == KEY_BACKSPACE)
            {
                if (cursor_pointer > 0) // is there any char to work with
                {
                    // go to previous char - char, which must be deleted
                    files_last_chr_indx[file_slot_indx][0]--;

                    // delete char/line
                    // change pntr_crr, depending on char/line deletion

                    // in any case char must be deleted
                    files_content[file_slot_indx][files_last_chr_indx[file_slot_indx][0]] == BLACK_COLOR;

                    // decide, how to set pntr_curr
                    
                    // new line char
                    if (files_content[file_slot_indx][files_last_chr_indx[file_slot_indx][0]] == '\n')
                    {
                        cursor_pointer--; // go to the end of prev line

                        // while there are no chars in framebuffer or start of row reached
                        while (*(framebuffer + (cursor_pointer - 1) * 2) == BLACK_COLOR && cursor_pointer % 80 != 0)
                        {
                            cursor_pointer--;
                        }
                    }
                    else // any other char
                    {
                        cursor_pointer--;
                    }

                    // clear deleted char data in framebuffer
                    *(framebuffer + cursor_pointer * 2) = BLACK_COLOR;
                    *(framebuffer + cursor_pointer * 2 + 1) = BLACK_COLOR << 4 | WHITE_COLOR;
                    // set cursor_pointer on deleted char place
                    put_cursor(cursor_pointer);
                }
            }
            else if (event.key == KEY_ENTER)
            {
                // 23 - unreachable last line to show full text when file is being readed
                if (cursor_pointer / 80 < 23)
                {
                    files_content[file_slot_indx][files_last_chr_indx[file_slot_indx][0]] = event.key_character;
                    print(&event.key_character, bcgr_color, YELLOW_COLOR);

                    files_last_chr_indx[file_slot_indx][0]++;
                }
            }
            // else if (event.key == KEY_TAB)
            // {
            //     files_content[file_slot_indx][files_last_chr_indx[file_slot_indx][0]] = '\0';
            //     edit_mode = false;

            //     clean_screen();
                
            //     recover_console_state();

            //     file_slot_indx = 0;

            //     char *new_line = "\n";
            //     print(new_line, BLACK_COLOR, BLACK_COLOR);
            //     print(line_char, BLACK_COLOR, GREEN_COLOR);
            // }
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

                    // *2 (для отримання доступ до символу а не до атребуту)
                    *(framebuffer + cursor_pointer * 2) = bcgr_color;   //при зміщені очищає попередній символ
                       
                    // *(framebuffer + cursor_pointer * 2 + 1) =  WHITE_COLOR;
                    put_cursor(cursor_pointer);
                }
            }
            
            else if (event.key == KEY_ENTER)
            {
                char cmnd_nmbr = 0;

                if (cursor_pointer % 80 > line_st_ofst)   // якщо було написано щось
                {
                    unsigned short position_text = cursor_pointer - cursor_pointer % 80 + line_st_ofst;  // знаходження позиції початкового тексту в поточному рядку (85-(85%80)+3  -> 85-5+3)

                    // while (*(framebuffer + position_text * 2) == ' ' &&
                    // position_text < cursor_pointer)
                    // {
                    //     // *(framebuffer + position_text * 2 + 1) = WHITE_COLOR;  
                    //     position_text++;
                    // }

                    if (*(framebuffer + position_text * 2) != bcgr_color)
                    {
                        cmnd_nmbr = search_command(&position_text);

                        if (cmnd_nmbr > -1)
                        {
                            // Command found

                            cmnds[cmnd_nmbr](&position_text);
                        }
                        else if (cmnd_nmbr == -1)
                        {
                            char *new_line = "\n";
                            print(new_line, bcgr_color, BLACK_COLOR);
                            char *msg_no_cmnd = "Command not found\n";
                            print(msg_no_cmnd, bcgr_color, RED_COLOR);
                        }
                    }
                    else
                    {
                        // тексту немає, лише пробіли
                    }
                }
                
                else
                {
                    char *new_line = "\n";
                    print(new_line, bcgr_color, BLACK_COLOR);
                }

                // edit -sleep
                // if (cmnd_nmbr != 5 && cmnd_nmbr != 2)
                // {
                //     print(line_char, bcgr_color, GREEN_COLOR);
                // }

                print(line_char, bcgr_color, GREEN_COLOR);
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
    unsigned short pos = current_row * MAX_COLS + current_col;
    if (TIMER_TICKES - KEY_PRESSED > MAX_DELAY) {
        if (!STARTED) {
            char *framebuff = (char *) 0xb8000;
            for (int i = 0; i < MAX_ROWS * MAX_COLS * 2; i += 2) {
                SAVE[i] = framebuff[i];       
                SAVE[i + 1] = framebuff[i + 1];
                framebuff[i] = ' ';            
                framebuff[i + 1] = 0x07;       
            }
            put_cursor(0); 
            STARTED = true;    
        }

        // Виводимо анімацію
        if (TIMER_TICKES % 10 == 0) {
            switch ((TIMER_TICKES / 10) % 3) {
                case 0:
                    print_char(20, 40, '(');
                    break;
                case 1:
                    print_char(20, 40, '|');
                    break;
                case 2:
                    print_char(20, 40, ')');
                    break;
            }
        }
    } else {
        if (STARTED) {
            char *framebuff = (char *) 0xb8000;
            for (int i = 0; i < MAX_ROWS * MAX_COLS * 2; i += 2) {
                framebuff[i] = SAVE[i];     
                framebuff[i + 1] = SAVE[i + 1];
            }
            STARTED = false; // Скидаємо анімацію
            put_cursor(pos);
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

