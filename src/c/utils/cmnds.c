#include "../kernel/kernel.h"
#include "../drivers/keyboard/keyboard.h"
#include "../drivers/timer/timer.h"
#include "../drivers/serial_port/serial_port.h"
#include "../utils/utils.h"

#define BLACK_COLOR 0x0
#define GREEN_COLOR 0xa
#define YELLOW_COLOR 0xe

char *cmnds_names[] = {
    "help",
    "clear",
    "sleep",
    "list",
    "create",
    "edit",
    "rename",
    "read",
    "delete",
};

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
                same = false;  // якщо не збіглися символи з команди та з веденого рядка (наприклад "l" != "k")
            
            cmnd_chrs_shift++;  // якщо перший симвло p vfcbde = "l" і другий сивмол з веденого текту = "l" то цикл продовжує порівнення
            pntr++;
        }

        if (same)
        {
            if (*(cmnds_names[cmnd_indx] + cmnd_chrs_shift) != '\0'){}

            else if(pntr == cursor_pointer || framebuffer[pntr * 2] == ' ')
            {
                *position_text = pntr;
                return cmnd_indx;
            }
        }
    }

    return -1;
};

void cmnd_clear()
{
    clean_screen();
    cursor_pointer = 0;
    char *ver_msg = "ChristOS - v 0.0.1\n";
    print(ver_msg, BLACK_COLOR, GREEN_COLOR);
}

void cmnd_help()
{
    print(new_line, BLACK_COLOR, BLACK_COLOR);
    char *msg = "\t   ======== Welcome to the system! ========\n\t   == You can use the following commands ==\n\t   help - show help message\n\t   clear - clean shell\n\t   sleep - screensave mode on\n\t   list - show tree of files\n\t   create - create new file\n\t   edit - edit the file\n\t   read - show file content\n\t   delete - delete file\n";
    print(msg, BLACK_COLOR, YELLOW_COLOR);
}

void cmnd_sleep()
{
    TIMER_TICKES = 1001;  
    KEY_PRESSED = 0;     
}

void cmnd_list()
{
    print(new_line, BLACK_COLOR, BLACK_COLOR);
    for (unsigned char slot_indx = 0; slot_indx < files_count; slot_indx++)
    {
        for (unsigned char cntr = 0; cntr < line_st_ofst; cntr++)
        {
            char *msg_space = " ";
            print(msg_space, BLACK_COLOR, BLACK_COLOR);
        }

        char *msg = "-> ";
        print(msg, BLACK_COLOR, YELLOW_COLOR);

        if (files_slots[slot_indx][0] == 1)
        {
            unsigned char shift_name = 0;
            print(&files_names[slot_indx][shift_name], BLACK_COLOR, YELLOW_COLOR);
        }

        print(new_line, BLACK_COLOR, YELLOW_COLOR);
    }
}

void cmnd_create(unsigned short *pntr_aftr_cmnd)
{
    unsigned short pntr = *pntr_aftr_cmnd;
    find_param_start(&pntr);
    unsigned char parameter[75];
    if(pntr == *pntr_aftr_cmnd || pntr == cursor_pointer) // Якщо не введено параметри
    {
        char *msg = "There is no parameter given\n";
        print_info_cmd(msg);
    }
    else if (pntr < cursor_pointer)  // Якщо введено параметри
    {
        read_param(&pntr, &parameter);

        bool same_name_file_exist = false;  //  Флаг для перевірки наявності файлу з таким ім'ям
        unsigned char same_name_file_indx = 0;   // Індекс наявного файлу з таким ім'ям

        find_file_name(&same_name_file_exist, &parameter, &same_name_file_indx);

        if (!same_name_file_exist) // Якщо імя не збігається і є новим
        {

            bool free_slot_exist = false;
            unsigned char free_slot = 0;
            for (unsigned char slot_indx = 0; slot_indx < files_count && !free_slot_exist; slot_indx++)
            {
                if (files_slots[slot_indx][0] == 0)
                {
                    free_slot_exist = true;
                    free_slot = slot_indx; // Запам'ятовуємо індекс вільного слота
                }
            }

            if (!free_slot_exist) // Якщо вільного лоту неамє
            {
                char *msg = "There are no slots avaliable for new file\n";
                print_info_cmd(msg);

            }
            else // Якщо вільний слот є, та створюємо новий файл
            {
                unsigned char shift_name = 0;  
                while (parameter[shift_name] != '\0')   // Копіюємо символи імені файлу
                {
                    files_names[free_slot][shift_name] = parameter[shift_name];
                    shift_name++;
                }
                files_slots[free_slot][0] = 1; // Встановлюємо значення 1, щоб позначити слот як зайнятий
                print(new_line, BLACK_COLOR, BLACK_COLOR);
                char *msg = "File created\n";
                print(msg, BLACK_COLOR, YELLOW_COLOR);
            }
        }
        else 
        {
            char *msg = "File with same name already exists\n";
            print_info_cmd(msg);
        }
    }
}

void cmnd_delete(unsigned short *pntr_aftr_cmnd)
{
    char *new_line = "\n";
    unsigned short pntr = *pntr_aftr_cmnd;

    find_param_start(&pntr);

    unsigned char parameter[75];

    if(pntr == *pntr_aftr_cmnd || pntr == cursor_pointer) // no pntr movement - cmnd just on line end
    {
        char *msg = "There is no parameter given\n";
        print_info_cmd(msg);
    }

    else if (pntr < cursor_pointer) 
    {
        read_param(&pntr, &parameter);
        bool same_name_file_exist = false;
        unsigned char same_name_file_indx = 0;
        find_file_name(&same_name_file_exist, &parameter, &same_name_file_indx);

        if (same_name_file_exist)
        {
            for (unsigned short shift_file = 0; shift_file < 2000; shift_file++)
            {
                files_content[same_name_file_indx][shift_file] = BLACK_COLOR;
            }
            for (unsigned char shift_name = 0; shift_name < 75; shift_name++)
            {
                files_names[same_name_file_indx][shift_name] = BLACK_COLOR;
            }
            files_last_chr_indx[same_name_file_indx][0] = 0;
            files_slots[same_name_file_indx][0] = 0;
            print(new_line, BLACK_COLOR, BLACK_COLOR);
            char *msg = "File was deleted\n";
            print(msg, BLACK_COLOR, YELLOW_COLOR);
        }
        else // no such a file name
        {
            char *msg = "File with such a name does not exist\n";
            print_info_cmd(msg);
        }
    }
}

void cmnd_rename(unsigned short *pntr_aftr_cmnd)
{
    unsigned short pntr = *pntr_aftr_cmnd;
    find_param_start(&pntr);

    unsigned char old_name[75];
    unsigned char new_name[75];

    read_param(&pntr, &old_name);
    find_param_start(&pntr); // Перехід до нового імені
    read_param(&pntr, &new_name);

    if (old_name[0] == '\0' || new_name[0] == '\0') // Якщо одне з імен не введено
    {
        char *msg = "Old or new file name is missing\n";
        print_info_cmd(msg);
        return;
    }

    bool old_name_exists, new_name_exists = false;
    unsigned char old_name_file_indx, new_name_file_indx = 0;
    find_file_name(&old_name_exists, &old_name, &old_name_file_indx);
    find_file_name(&new_name_exists, &new_name, &new_name_file_indx);

    if (!old_name_exists || new_name_exists) 
    {
        char *msg = "File with old or new name does not exist\n";
        print_info_cmd(msg);
    }
    else 
    {
        unsigned char shift_name = 0;
        while (files_names[old_name_file_indx][shift_name] != '\0') {
            files_names[old_name_file_indx][shift_name] = '\0';
            shift_name++;
        }

        shift_name = 0; 
        while (new_name[shift_name] != '\0') {        // Копіюємо нове ім'я у відповідний індекс
            files_names[old_name_file_indx][shift_name] = new_name[shift_name];
            shift_name++;
        }
        print(new_line, BLACK_COLOR, BLACK_COLOR);
        char *msg = "File renamed successfully\n";
        print(msg, BLACK_COLOR, YELLOW_COLOR);
    }
}

void cmnd_read(unsigned short *pntr_aftr_cmnd)
{
    unsigned short pntr = *pntr_aftr_cmnd;

    find_param_start(&pntr);

    unsigned char parameter[75];

    if(pntr == *pntr_aftr_cmnd || pntr == cursor_pointer) // no pntr movement - cmnd just on line end
    {
        char *msg = "There is no parameter given\n";
        print_info_cmd(msg);
    }
    else if (pntr < cursor_pointer) // were spaces, and some chars after spaces exists - parameter. save it
    {
        read_param(&pntr, &parameter);

        // answer the question: is there any file with such a name?

        bool same_name_file_exist = false;
        unsigned char same_name_file_indx = 0;
        // search for same name
        // code
        find_file_name(&same_name_file_exist, &parameter, &same_name_file_indx);

        if (same_name_file_exist) // if file is found - show its content by saved index
        {
            print(new_line, BLACK_COLOR, BLACK_COLOR);
            char *msg = &(files_content[same_name_file_indx]);
            print(msg, BLACK_COLOR, YELLOW_COLOR);

            print(new_line, BLACK_COLOR, BLACK_COLOR);
        }
        else // no such a file name
        {
            char *msg = "File with such a name does not exist\n";
            print_info_cmd(msg);
        }
    }
}

void cmnd_edit(unsigned short *pntr_aftr_cmnd)
{
    unsigned short pntr = *pntr_aftr_cmnd;

    find_param_start(&pntr);

    unsigned char parameter[75];

    if(pntr == *pntr_aftr_cmnd || pntr == cursor_pointer) // no pntr movement - cmnd just on line end or were spaces, but still no parameter
    {
        print(new_line, BLACK_COLOR, BLACK_COLOR);
        char *msg = "There is no parameter given\n";
        print_info_cmd(msg);
        print(line_char, BLACK_COLOR, YELLOW_COLOR);
    }
    else if (pntr < cursor_pointer) 
    {
        read_param(&pntr, &parameter);

        bool same_name_file_exist = false;
        unsigned char same_name_file_indx = 0;
        find_file_name(&same_name_file_exist, &parameter, &same_name_file_indx);

        if (same_name_file_exist) // if file is found - go to edit mode
        {
            edit_mode = true;
            save_console_state();
            clean_screen();
            file_slot_indx = same_name_file_indx;

            char *msg = &(files_content[same_name_file_indx]);
            print(msg, BLACK_COLOR, YELLOW_COLOR);

            files_content[same_name_file_indx][files_last_chr_indx[file_slot_indx][0]] = BLACK_COLOR;
        }
        else 
        {
            char *msg = "File with such a name does not exist\n";
            print_info_cmd(msg);
        }
    }
}