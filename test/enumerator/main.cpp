#include <iostream>

#include <moducom/libusb/diagnostic.hpp>
#include <moducom/libusb.h>

using namespace moducom;
using namespace moducom::services;

int main()
{
    LibUsb libusb;

    auto devices = libusb.registry.view<LibUsb::Device>();

    std::cout.fill('0');

    devices.each([&](entt::entity e, const LibUsb::Device& device)
    {
        const moducom::libusb::ConfigDescriptor* _config = libusb.registry.try_get<moducom::libusb::ConfigDescriptor>(e);

        // NOTE: Not putting this into dump because occasionally a glitch seems to happen which
        // disables device from running after - might have just been bad housekeeping on device handle
        // though
        try
        {
            Scoped<libusb::DeviceHandle> dh = device.open();

            dh->set_auto_detach_kernel_driver(true);

            std::cout << std::endl;
            std::cout << "Manufacturer: ";
            std::cout << dh->get_string_descriptor(device.descriptor().iManufacturer) << std::endl;
            std::cout << "Product: " << dh->get_string_descriptor(device.descriptor().iProduct);
            std::cout << std::endl;
        }
        catch(const libusb::Exception& e)
        {
            //std::clog << "Couldn't open device addr=" << (int)device.address() << std::endl;
        }

        moducom::diagnostic::dump(std::cout, device, _config);
    });

    return 0;
}
