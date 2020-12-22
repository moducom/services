#include <iostream>

#include <ncurses.h>
#include <moducom/libusb/service.hpp>
#include "../../src/service.libusb/acm.h"

#define ENABLE_NCURSES 0
#define ENABLE_GETINFO 0

typedef moducom::libusb::services::experimental::CP210xTraits CP210xTraits;

typedef moducom::libusb::services::Transfer
        <moducom::libusb::services::internal::TransferEnttImpl> Transfer;

class Session
{
    moducom::libusb::Guard<moducom::libusb::DeviceHandle> handle;
    Transfer in;

    void render(moducom::libusb::Transfer& transfer)
    {
        char buf[2048];

        // FIX: Looks like we are getting the same buffer over and over again,
        // so I think the flags to Transfer aren't allowing it to proceed forward
        moducom::libusb::Buffer buffer = transfer.buffer();

        std::copy(buffer.buffer, buffer.buffer + buffer.length, buf);
        buf[buffer.length] = 0;

        // DEBT: Do this in a different thread
#if ENABLE_NCURSES
        printw(buf);
#else
        std::cout << buf;
#endif
    }

public:
    Session(moducom::libusb::Device device) :
        handle(device),
        in(*handle, CP210xTraits::inEndpoint, 1024)
    {
        // DEBT: Linux only, wrap with an #ifdef
        handle->set_auto_detach_kernel_driver(true);

        handle->claim_interface(0);
        in.impl.sinkCompleted.connect<&Session::render>(this);
        in.alloc();
        in.start();
    }

    ~Session()
    {
        // FIX: Needs better than this to stop, needs some time to stop too I think
        in.stop();

        handle->release_interface(0);
        in.impl.sinkCompleted.disconnect();
    }
};


Session* session = nullptr;

void found(entt::registry& r, entt::entity e)
{
    auto& device = r.get<moducom::services::LibUsb::Device>(e);

#if ENABLE_GETINFO
    try
    {
        moducom::libusb::DeviceHandle dh = device.device->open();

        std::string productName;

        try
        {
            auto test = dh.get_u16string_descriptor(device.device_descriptor.iProduct, 2);
            productName = dh.get_string_descriptor(device.device_descriptor.iProduct);
        }
        catch(const moducom::libusb::Exception& e)
        {
            std::cerr << "Could not get product name: " << e.what() << std::endl;
        }

        std::cout << "Product name = " + productName << "." << std::endl;

        printw("Device");
        nl();
        refresh();

        dh.close();
    }
    catch(const moducom::libusb::Exception& e)
    {
    }
#endif

    // DEBT: Not endian-friendly, but I lost our little helper for that
    if(device.device_descriptor.idVendor == CP210xTraits::VID &&
       device.device_descriptor.idProduct == CP210xTraits::PID)
    {
        session = new Session(*device.device);
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

    //std::cout << "Hello, World!" << std::endl;

    timeout(0);
    while(getch() != 'x')
    {
        // DEBT: run() blocks, but eventually shouldn't
        libusb.run();
        refresh();
    }

    if(session) delete session;

    endwin();

    return 0;
}
