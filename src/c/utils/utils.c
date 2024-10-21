#include "../kernel/kernel.h"
#include "../drivers/keyboard/keyboard.h"
#include "../drivers/timer/timer.h"
#include "../drivers/serial_port/serial_port.h"

#define VIDEO_MEMORY 0xB8000
#define VGA_WIDTH 80
#define MAX_ROWS 25
#define MAX_COLS 80

extern char *framebuffer;
// int command_count = sizeof(commands) / sizeof(commands[0]);
extern short cursor_pointer;
extern short TIMER_TICKES;

#define BLACK_COLOR 0x0
#define GREEN_COLOR 0xa
#define YELLOW_COLOR 0xe
#define RED_COLOR 0xc
#define WHITE_COLOR 0xf
#define PINK_COLOR 0xd
unsigned char cnsl_state_clrs[25 * 80];

char *cmnds_names[] = {
    "help",
    "clear",
    "sleep",
    "list",
    // "create",
    // "edit",
    // "read",
    // "delete",
    // "flip"
};

void print_char(int row, int col, char c) {
    unsigned short position = (row * VGA_WIDTH + col) * 2;
    volatile char *video = (volatile char *)VIDEO_MEMORY;
    video[position] = c;
    video[position + 1] = 0x0A; 
}

// void move_to_new_line(unsigned short *current_row, unsigned short *current_col, unsigned short max_rows, bool *NEW_LINE) {
//     (*current_row)++;  // Збільшуємо номер рядка
//     *current_col = 0;  // Скидаємо номер стовпця на 0

//     if (*current_row >= max_rows) {
//         *current_row = max_rows - 1; // Обмежуємо рядок, якщо досягнуто межі
//         // *NEW_LINE = false;
//     }
// }

void new_line_handle(unsigned short *current_row, unsigned short *current_col, bool *NEW_LINE) {
    if (NEW_LINE) {
        print_char(*current_row, 0, '>'); 
        print_char(*current_row, 1, ' '); 
        *current_col = 2; 
        *NEW_LINE = false;
    }
}

void clean_screen(){
        char *framebuff = (char *)0xb8000;
    for (int i = 0; i < MAX_ROWS * MAX_COLS * 2; i += 2) {
        framebuff[i] = ' ';
        framebuff[i + 1] = 0x07;
    }
}

void print(char *msg, unsigned char clr_bckg, unsigned char clr_fnt)
{
    while (*msg != '\0')
    {
        if (*msg == '\n')
        {
            cursor_pointer += 80 - cursor_pointer % 80;
        }
        else
        {
            *(framebuffer + cursor_pointer * 2) = *msg;
            *(framebuffer + cursor_pointer * 2 + 1) = (clr_bckg << 4) | clr_fnt;
            cursor_pointer ++;
        }
        msg++;
        
        if (cursor_pointer > 1999)
        {
            scroll();
        }
        else
        {
            put_cursor(cursor_pointer);
        }
    }
}

void scroll()
{
    for (unsigned char rw = 0; rw < 24; rw++)
    {
        for (unsigned char clmn = 0; clmn < 80; clmn++)
        {
            *(framebuffer + rw * (80 * 2) + clmn * (2)) = 
            *(framebuffer + (rw + 1) * (80 * 2) + clmn * (2));

            *(framebuffer + rw * (80 * 2) + clmn * (2) + 1) = 
            *(framebuffer + (rw + 1) * (80 * 2) + clmn * (2) + 1);
        }
    }

    for (unsigned char clmn = 0; clmn < 80; clmn++)
    {
        *(framebuffer + 24 * (80 * 2) + clmn * (2)) = 0x0;
        *(framebuffer + 24 * (80 * 2) + clmn * (2) + 1) = 0xf;
    }
    
    cursor_pointer = 80 * 24;
    put_cursor(cursor_pointer);
}

void cmnd_clear()
{
    clean_screen();
    cursor_pointer = 0;
    char *ver_msg = "ChristOS - v 0.0.1\n";
    print(ver_msg, BLACK_COLOR, GREEN_COLOR);
}

void cmnd_help()
{
    char *new_line = "\n";
    print(new_line, BLACK_COLOR, BLACK_COLOR);
    
    char *msg_help = "\t   ======== Welcome to the system! ========\n\t   == You can use the following commands ==\n\t   help - show help message\n\t   clear - clean shell\n\t   sleep - screensave mode on\n\t   list - show tree of files\n\t   create - create new file\n\t   edit - edit the file\n\t   read - show file content\n\t   delete - delete file\n";
    print(msg_help, BLACK_COLOR, YELLOW_COLOR);
}

void cmnd_sleep()
{
    TIMER_TICKES = 200;
    timer_tick_handler();
}

char search_command(unsigned short *position_text)
{
    unsigned short cmnds_nums = sizeof(cmnds_names) / sizeof(cmnds_names[0]);   // обчислюємо кількість елементів (команд) у масиві
    for (unsigned short cmnd_indx = 0; cmnd_indx < cmnds_nums; cmnd_indx++)     // перевірте кожну команду
    {
        unsigned short pntr = *position_text;
        bool same = true;           
        unsigned short cmnd_chrs_shift = 0;
        while(*(cmnds_names[cmnd_indx] + cmnd_chrs_shift) != '\0' && pntr < cursor_pointer && same)
        {
            if (*(cmnds_names[cmnd_indx] + cmnd_chrs_shift) != *(framebuffer + pntr * 2))
            {
                same = false;  // якщо не збіглися символи з команди та з веденого рядка (наприклад "l" != "k")
            }
            
            cmnd_chrs_shift++;  // якщо перший симвло p vfcbde = "l" і другий сивмол з веденого текту = "l" то цикл продовжує порівнення
            pntr++;
        }

        if (same)
        {
            if (*(cmnds_names[cmnd_indx] + cmnd_chrs_shift) != '\0')
            {
                // частина назви команди збігається, але не вся назва
            }
            else if(pntr == cursor_pointer || framebuffer[pntr * 2] == ' ')
            {
                *position_text = pntr;
                return cmnd_indx;
            }
        }
    }

    return -1;
};

