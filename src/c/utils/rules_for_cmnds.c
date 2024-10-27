#include "../kernel/kernel.h"
#include "../drivers/keyboard/keyboard.h"
#include "../drivers/timer/timer.h"
#include "../drivers/serial_port/serial_port.h"
#include "../utils/utils.h"

#define BLACK_COLOR 0x0
#define GREEN_COLOR 0xa
#define YELLOW_COLOR 0xe
#define RED_COLOR 0xc
#define WHITE_COLOR 0xf
#define PINK_COLOR 0xd

unsigned char cnsl_state_chrs[25 * 80];
unsigned char cnsl_state_clrs[25 * 80];
unsigned short cnsl_state_pntr = 0;

void save_console_state()
{
    cnsl_state_pntr = cursor_pointer;

    cursor_pointer = 0;

    put_cursor(cursor_pointer);

    for (unsigned char row = 0; row < 25; row++)
    {
        for (unsigned char col = 0; col < 80; col++)
        {
            cnsl_state_chrs[row * 80 + col] = 
                *(framebuffer + row * (80 * 2) + col * (2));

            cnsl_state_clrs[row * 80 + col] = 
                *(framebuffer + row * (80 * 2) + col * (2) + 1);
        }
    }
}

void find_param_start(unsigned short *pntr) {
    while (*(framebuffer + (*pntr) * 2) == ' ' && (*pntr) < cursor_pointer) {
        *(framebuffer + (*pntr) * 2 + 1) = WHITE_COLOR;
        (*pntr)++;        // Збільшуємо значення вказівника, переходячи до наступної позиції
    }
}

void find_file_name(bool *same_name_file_exist, unsigned char *parameter, unsigned char *same_name_file_indx) {
    for (unsigned char file_indx = 0; file_indx < files_count && !(*same_name_file_exist); file_indx++) {  // Перебираємо всі файли, щоб знайти файл з таким же іменем
        unsigned char shift_name = 0;

        while (files_names[file_indx][shift_name] == parameter[shift_name] &&
         files_names[file_indx][shift_name] != '\0' &&
          parameter[shift_name] != '\0') {   // Змінна для порівняння символів імені файлу
            shift_name++;  // Переходимо до наступного символу
        }

        if (files_names[file_indx][shift_name] == '\0' && parameter[shift_name] == '\0') {  // Якщо було оночасно закінчена перевірка, це означає що таке ім'я вже було зарезервоване
            *same_name_file_exist = true;  // Встановлюємо прапорець, що файл існує
            *same_name_file_indx = file_indx;  // Запам'ятовуємо індекс файлу
        }
    }
}

void read_param(unsigned short *pntr, unsigned char *parameter)
{
    unsigned char param_indx = 0;
    while (*(framebuffer + (*pntr) * 2) != ' ' && (*pntr) < cursor_pointer)
    {
        parameter[param_indx] = *(framebuffer + (*pntr) * 2); // Зберігаємо символ у параметрі

        param_indx++; // Збільшуємо індекс для наступного символу
        (*pntr)++;   // Переходимо до наступної позиції
    }
    parameter[param_indx] = '\0';  // Завершуємо рядок нульовим символом
} 

void print_info_cmd(char *msg){
    int i = 0;
    while (msg[i] != '\0') {
        // Тут можна додати логіку для діагностики кожного символа, якщо необхідно
        i++;
    }

    // Далі логіка для виводу в кольорі
    print(new_line, bcgr_color, BLACK_COLOR);
    print(msg, bcgr_color, RED_COLOR);
}


// void delete_exist_file(unsigned short pntr){
//         unsigned char parameter[75];
//         read_param(&pntr, &parameter);

//         bool same_name_file_exist = false;
//         unsigned char same_name_file_indx = 0;
//         // search for same name
//         find_file_name(&same_name_file_exist, &parameter, &same_name_file_indx);

//         if (same_name_file_exist) // if file is found - go to edit mode
//         {
//             save_console_state();
//             clean_screen();
//             edit_mode = true;
//             file_slot_indx = same_name_file_indx;

//             char *msg = &(files_content[same_name_file_indx]);
//             print(msg, BLACK_COLOR, YELLOW_COLOR);

//             files_content[same_name_file_indx][files_last_chr_indx[file_slot_indx][0]] = BLACK_COLOR;
//         }
//         else // no such a file name
//         {
//             print(new_line, BLACK_COLOR, BLACK_COLOR);
//             char *msg = "File with such a name does not exist\n";
//             print(msg, BLACK_COLOR, RED_COLOR);
//             print(line_char, BLACK_COLOR, GREEN_COLOR);
//         }
// }