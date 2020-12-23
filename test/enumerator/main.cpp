#include <iostream>

#include <moducom/libusb/diagnostic.hpp>
#include <moducom/libusb.h>

using namespace moducom::services;

int main()
{
    LibUsb libusb;

    auto devices = libusb.registry.view<LibUsb::Device>();

    devices.each([&](entt::entity e, const auto& device)
    {
        const moducom::libusb::ConfigDescriptor* _config = libusb.registry.try_get<moducom::libusb::ConfigDescriptor>(e);

        moducom::diagnostic::dump(std::cout, device, _config);
    });

    return 0;
}
