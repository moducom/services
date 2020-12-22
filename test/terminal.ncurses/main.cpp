#include <iostream>

#include <ncurses.h>
#include <moducom/libusb/service.hpp>
#include "../../src/service.libusb/acm.h"

int main()
{
    moducom::services::LibUsb libusb;
    
    initscr();
    printw("Hello World !!!");
    refresh();
    getch();
    endwin();
    //std::cout << "Hello, World!" << std::endl;
    return 0;
}
