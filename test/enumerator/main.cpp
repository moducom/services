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

        moducom::diagnostic::dump(std::cout, device, _config);

        /* various access issues
        Scoped<libusb::DeviceHandle> dh = device.open();

        dh->set_auto_detach_kernel_driver(true);

        std::cout << "Vendor: " << dh->get_string_descriptor(device.descriptor().idVendor); */
    });

    return 0;
}
