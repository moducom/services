#include <iostream>

#include <ncurses.h>
#include <moducom/libusb/service.hpp>
#include "../../src/service.libusb/acm.h"

void found(entt::registry& r, entt::entity e)
{
    auto& device = r.get<moducom::services::LibUsb::Device>(e);

    printw("Device");
    nl();
    refresh();
}

int main()
{
    moducom::services::LibUsb libusb(false);

    initscr();

    libusb.registry.on_construct<moducom::services::LibUsb::Device>().connect<&found>();

    libusb.init();

    printw("Hello World !!!");
    refresh();
    getch();
    endwin();
    //std::cout << "Hello, World!" << std::endl;
    return 0;
}
