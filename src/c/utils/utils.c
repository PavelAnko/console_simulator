#include "../kernel/kernel.h"
#include "../drivers/keyboard/keyboard.h"
#include "../drivers/timer/timer.h"
#include "../drivers/serial_port/serial_port.h"
#include "../utils/utils.h"

#define VIDEO_MEMORY 0xB8000
#define VGA_WIDTH 80
#define MAX_ROWS 25
#define MAX_COLS 80

#define BLACK_COLOR 0x0
#define GREEN_COLOR 0xa
#define YELLOW_COLOR 0xe
#define RED_COLOR 0xc
#define WHITE_COLOR 0xf
#define PINK_COLOR 0xd
#define LIGHT_GREEN_COLOR 0xa 
unsigned char cnsl_state_clrs[25 * 80];

static unsigned long next = 1;

void print_char(int row, int col, char c) {
    unsigned short position = (row * VGA_WIDTH + col) * 2;
    volatile char *video = (volatile char *)VIDEO_MEMORY;
    video[position] = c;
    video[position + 1] = 0x0A; 
}

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
            cursor_pointer += 80 - cursor_pointer % 80;
        else
        {
            *(framebuffer + cursor_pointer * 2) = *msg;
            *(framebuffer + cursor_pointer * 2 + 1) = (clr_bckg << 4) | clr_fnt;
            cursor_pointer ++;
        }
        msg++;
        
        if (cursor_pointer > 1999)  
            scroll(); 
        else
            put_cursor(cursor_pointer);
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

int rand() {
    next = next * 1103515245 + 12345;  // Лінійний конгруентний генератор
    return (unsigned int)(next / 65536) % 32768;  // Повертаємо значення в діапазоні [0, 32767]
}

void recover_console_state()
{
    unsigned short anim_offsite_X = 0;
    unsigned short anim_offsite_Y = 0;
    unsigned short anim_coord_X = 1;
    unsigned short anim_coord_Y = 1;

    cursor_pointer = cnsl_state_pntr;

    cnsl_state_pntr = 0;

    put_cursor(cursor_pointer);

    for (unsigned char row = 0; row < 25; row++)
    {
        for (unsigned char col = 0; col < 80; col++)
        {
            *(framebuffer + row * (80 * 2) + col * (2)) = cnsl_state_chrs[row * 80 + col];
            cnsl_state_chrs[row * 80 + col] = BLACK_COLOR;

            *(framebuffer + row * (80 * 2) + col * (2) + 1) = cnsl_state_clrs[row * 80 + col];
            cnsl_state_clrs[row * 80 + col] = BLACK_COLOR;
        }
    }
}

void save_console_txt(){
    if (!STARTED) {
        // Зберігаємо вміст екрану
        for (int i = 0; i < MAX_ROWS * MAX_COLS * 2; i += 2) {
            SAVE[i] = framebuff[i];       // Збереження символу
            SAVE[i + 1] = framebuff[i + 1]; // Збереження кольору символу
            framebuff[i] = ' ';            // Очищення екрана
            framebuff[i + 1] = 0x07;       // Білий текст на чорному фоні
        }

        put_cursor(-1);  // Приховуємо курсор
        STARTED = true;  // Переходимо в режим анімації
    }
}

void start_animation(){
    static int columns[MAX_COLS]; // Позиція "крапель" у кожній колонці
    static int init = 0;

    // Ініціалізуємо колонки, якщо це перший кадр
    if (!init) {
        for (int i = 0; i < MAX_COLS; i++) {
            columns[i] = rand() % MAX_ROWS;  // Випадковий рядок для кожної колонки
        }
        init = 1;
    }

    // Оновлюємо екран
    char digits[] = "0123456789";
    int colors[] = {GREEN_COLOR, LIGHT_GREEN_COLOR};  // Кольори для "цифрового дощу"

    for (int col = 0; col < MAX_COLS; col++) {
        int row = columns[col];

        // Очищуємо попередній символ у цій колонці
        if (row > 0) {
            *(framebuff + ((row - 1) * MAX_COLS + col) * 2) = ' ';
            *(framebuff + ((row - 1) * MAX_COLS + col) * 2 + 1) = 0x07;
        }

        // Виводимо нову цифру
        char digit = digits[rand() % 10];
        int color = colors[rand() % 2];

        *(framebuff + (row * MAX_COLS + col) * 2) = digit;
        *(framebuff + (row * MAX_COLS + col) * 2 + 1) = color;

        // Зсуваємо "краплю" вниз
        columns[col] = (row + 1) % MAX_ROWS;
    }
}