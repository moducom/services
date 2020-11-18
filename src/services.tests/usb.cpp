#include "catch2/catch.hpp"
#include "services.h"

#include <service.libusb/usb.h>
#include <service.libusb/acm.h>

using namespace moducom::services;

constexpr uint16_t VID_CP210x = 0x10c4;
constexpr uint16_t PID_CP210x = 0xea60;

TEST_CASE("usb")
{
    using namespace std::chrono_literals;

    LibUsb libusb;
    // DEBT: Gonna get limited mileage out of a null-initialized device handle
    moducom::libusb::DeviceHandle deviceHandle(nullptr);

    SECTION("acm")
    {
        auto desciptors = libusb.registry.view<libusb_device_descriptor>();

        auto result = std::find_if(std::begin(desciptors), std::end(desciptors), [&](auto& entity)
        {
            libusb_device_descriptor& deviceDescriptor = desciptors.get<libusb_device_descriptor>(entity);

            if(deviceDescriptor.idVendor == htole16(VID_CP210x) &&
                deviceDescriptor.idProduct == htole16(PID_CP210x))
            {
                return true;
            }

            return false;
        });

        if(result != std::end(desciptors))
        {
            libusb_device* device = libusb.registry.get<libusb_device*>(*result);
            moducom::libusb::Device _device(device);
            moducom::libusb::DeviceHandle dh = _device.open();
            constexpr int interface_number = 0;

            if(dh.kernel_driver_active(interface_number))
                dh.detach_kernel_driver(interface_number);

            dh.claim_interface(interface_number);

            AcmLibUsb acm1(dh);

            acm1.setLineCoding(115200);

            // .run waits for 5s each time
            for(int counter = 60 / 5; counter--;)
            {
                libusb.run();
                std::this_thread::sleep_for(1s);
            }

            dh.release(interface_number);
            libusb_attach_kernel_driver(dh, interface_number);

            dh.close();
        }

        AcmLibUsb acm(deviceHandle);
    }
}