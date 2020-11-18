#include "catch2/catch.hpp"
#include "services.h"

#include <service.libusb/usb.h>
#include <service.libusb/acm.h>

using namespace moducom::services;

constexpr uint16_t VID_CP210x = 0x10c4;
constexpr uint16_t PID_CP210x = 0xea60;

TEST_CASE("usb")
{
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

            AcmLibUsb acm1(dh);

            dh.close();
        }

        AcmLibUsb acm(deviceHandle);
    }
}