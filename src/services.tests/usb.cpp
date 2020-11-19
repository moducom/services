#include "catch2/catch.hpp"
#include "services.h"

#include <service.libusb/usb.h>
#include <service.libusb/acm.h>

using namespace moducom::services;

constexpr uint16_t VID_CP210x = 0x10c4;
constexpr uint16_t PID_CP210x = 0xea60;

static std::future<void> async_result;

// since this is called on the USB event "thread", we need to get in and out of here asap
void printer(moducom::libusb::Buffer buffer)
{
    auto s = std::make_unique<std::string>((const char*)buffer.buffer, buffer.length);

    // At least we get a pseudo-queue of one async process by doing this
    async_result = std::async(std::launch::async, [](auto _s)
    {
        std::cout << *_s;
        //"sz = " << length << std::endl;
    }, std::move(s));
}

TEST_CASE("usb")
{
    entt::registry registry;
    agents::EnttHelper eh(registry, registry.create());
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

            dh.set_auto_detach_kernel_driver(true);

            // so that AcmLibUsb spins down before dh.close() is called
            {
                // Hardcoded for CP210x
                constexpr uint8_t inEndpoint = 0x82;
                constexpr uint8_t outEndpoint = 0x01;

                AcmLibUsb acm1(dh, inEndpoint, outEndpoint);

                acm1.setLineCoding(115200);

                acm1.sinkTransferReceived.connect<printer>();

                // .run waits for 5s each time
                libusb.run();
                libusb.run();
            }

            dh.close();
        }

        AcmLibUsb acm(deviceHandle, 0, 0);
    }
}