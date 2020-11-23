#include "catch2/catch.hpp"
#include "services.h"

#include <service.libusb/usb.h>
#include <service.libusb/acm.h>

using namespace moducom::services;

// In my case, for ESP32 CP2104
struct CP210x
{
    static constexpr uint16_t VID = 0x10c4;
    static constexpr uint16_t PID = 0xea60;
    static constexpr uint8_t inEndpoint = 0x82;
    static constexpr uint8_t outEndpoint = 0x01;
};

static std::future<void> async_result;

// For the sake of running REAL unit tests vs abusing them as
// experimentation and integration tests
#define ENABLE_LIVE_USB_TEST 1

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

    auto desciptors = libusb.registry.view<libusb_device_descriptor>();

    auto result = std::find_if(std::begin(desciptors), std::end(desciptors), [&](auto& entity)
    {
        libusb_device_descriptor& deviceDescriptor = desciptors.get<libusb_device_descriptor>(entity);

        if(deviceDescriptor.idVendor == htole16(CP210x::VID) &&
           deviceDescriptor.idProduct == htole16(CP210x::PID))
        {
            return true;
        }

        return false;
    });

    SECTION("acm")
    {
#if DISABLED_ENABLE_LIVE_USB_TEST
        if(result != std::end(desciptors))
        {
            libusb_device* device = libusb.registry.get<libusb_device*>(*result);
            moducom::libusb::Device _device(device);
            moducom::libusb::DeviceHandle dh = _device.open();

            dh.set_auto_detach_kernel_driver(true);

            // so that AcmLibUsb spins down before dh.close() is called
            {
                AcmLibUsb acm1(dh, CP210x::inEndpoint, CP210x::outEndpoint);

                // BUGGED: Sends (nothing?) out over 'out' when should be sending over 'control'
                // - listening doesn't require anyway, so leaving to fix another day
                //acm1.setLineCoding(115200);

                acm1.sinkTransferReceived.connect<printer>();

                // .run waits for 5s each time
                libusb.run();
                libusb.run();
            }

            dh.close();
        }
#endif

        AcmLibUsb acm(deviceHandle, 0, 0);
    }
    SECTION("bulk")
    {
        // Not working yet, presumably because I don't set line coding yet
#if ENABLE_LIVE_USB_TEST
        if(result != std::end(desciptors))
        {
            libusb_device* device = libusb.registry.get<libusb_device*>(*result);
            moducom::libusb::Device _device(device);
            moducom::libusb::DeviceHandle dh = _device.open();

            dh.set_auto_detach_kernel_driver(true);

            {
                LibUsbTransferIn in(eh, dh, CP210x::inEndpoint, 32);

                auto& sink = eh.get<entt::sink<void (moducom::libusb::Buffer)> >();

                sink.connect<printer>();

                in.start();

                libusb.run();
                libusb.run();

                in.stop();
            }

            dh.close();
        }
#endif
    }
}