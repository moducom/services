#include <iostream>

#include <ncurses.h>
#include <moducom/libusb/service.hpp>
#include "../../src/service.libusb/acm.h"

#define ENABLE_NCURSES 0

void found(entt::registry& r, entt::entity e)
{
    auto& device = r.get<moducom::services::LibUsb::Device>(e);

    try
    {
        moducom::libusb::DeviceHandle dh = device.device.open();

        char buf[128];

        try
        {
            dh.get_string_descriptor(device.device_descriptor.iProduct, buf, sizeof(buf));
        }
        catch(const moducom::libusb::Exception& e)
        {

        }

        printw("Device");
        nl();
        refresh();

        dh.close();
    }
    catch(const moducom::libusb::Exception& e)
    {
    }
}

int main()
{
    moducom::services::LibUsb libusb(false);

#if ENABLE_NCURSES
    initscr();
#endif

    libusb.registry.on_construct<moducom::services::LibUsb::Device>().connect<&found>();

    libusb.init();

    printw("Hello World !!!");
    refresh();
    getch();
    endwin();
    //std::cout << "Hello, World!" << std::endl;
    return 0;
}
