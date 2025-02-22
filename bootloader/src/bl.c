#include <stdio.h>
#include "ncurses.h"
#include "comm.h"
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

int main_cols, main_lines;
int main_cols_cur, main_lines_cur;
WINDOW *mainwin;	
WINDOW *menuwin;	
WINDOW *infowin;	

static void redraw(void)
{
    mainwin = initscr();	
    getmaxyx(stdscr, main_lines, main_cols);
    int menu_win_cols = main_cols /3;
    int menu_win_lines = 2 * main_lines / 3;
    menuwin = subwin(mainwin, menu_win_lines, menu_win_cols, main_lines/2 - menu_win_lines/2, main_cols/4 - menu_win_cols/2);
    infowin = subwin(mainwin, menu_win_lines, menu_win_cols, main_lines/2 - menu_win_lines/2, 3*main_cols/4 - menu_win_cols/2);
    box(mainwin, 0, 0);
    box(menuwin, 0, 0);
    wprintw(mainwin, " Bootloader ");	
    wprintw(menuwin, " Watcha wanna do? ");

    keypad(menuwin, true);

}

typedef struct {
    uint8_t number;
    char *desc;
} Menu_t;

#define item_no 8

Menu_t mainmenu[item_no] = {
    {.number = COMMAND_PR, .desc = "- Read parameters [1]" } ,
    {.number = COMMAND_PU, .desc = "- Update parameters [2]" } ,
    {.number = COMMAND_RFI, .desc = "- Read firmware info [3]" } ,
    {.number = COMMAND_RST, .desc = "- Update the firmware [4]" } ,
    {.number = COMMAND_LOCK, .desc = "- Lock serial line [5]" } ,
    {.number = COMMAND_UNLOCK, .desc = "- Unlock serial line [6]" } ,
    {.number = COMMAND_RDIAG, .desc = "- Read diagnostics [7]" } ,
    {.number = COMMAND_QUIT, .desc = "- Quit [0]" } ,
};

int main(int argc, char **argv)
{
    char *serialPort = "/dev/ttyUSB0";
    char response[1024] = {0};
    redraw();
    comm_open_serial_port(serialPort, response);
    mvwprintw(infowin, 3, 0, response);
    wrefresh(infowin);			
    wrefresh(mainwin);		

    (void)argc;
    (void)argv;

    redraw();

    int choice = 1;
    uint8_t highlight = 0;
    uint8_t option = 0;

    while (option != item_no - 1) {

        getmaxyx(stdscr, main_lines_cur, main_cols_cur);
        if (main_lines_cur != main_lines || main_cols_cur != main_cols) {
            werase(mainwin);
            werase(infowin);
            werase(menuwin);
            redraw();
            main_lines = main_lines_cur;
            main_cols = main_cols_cur;
        }
        for(uint8_t i = 0; i < item_no; i++) {
            if (i == highlight) wattron(menuwin, A_REVERSE);
            mvwprintw(menuwin, i + 2, 2, mainmenu[i].desc);
            wattroff(menuwin, A_REVERSE);
        }
        wrefresh(menuwin);			
        wrefresh(mainwin);		
        wrefresh(infowin);
        choice = wgetch(menuwin);
        switch(choice) {
            case KEY_UP:
                highlight--;
                if (highlight == 0xff) highlight = item_no - 1;
                break;
            case KEY_DOWN:
                highlight = (highlight + 1) % item_no;
                break;
            default:
                break;
        }
        if (choice == 10) {
            option = highlight;
            wclear(infowin);
            char buffer[255];
            sprintf(buffer, "Command: %s", mainmenu[highlight].desc);
            mvwprintw(infowin, 1, 0, buffer);
            if (mainmenu[highlight].number == COMMAND_RST) {
                sprintf(buffer, "Update in progress ... ");
                mvwprintw(infowin, 2, 1, buffer);
            }
            wrefresh(infowin);			
            uint16_t len = execute_command(mainmenu[highlight].number, response);
            response[len] = '\0';
            mvwprintw(infowin, 3, 0, response);
            wrefresh(infowin);			
            wrefresh(mainwin);		
        }
    }

    comm_close_serial_port();

    endwin();
    return 0;
}
