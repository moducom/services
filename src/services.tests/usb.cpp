#include "catch2/catch.hpp"
#include "services.h"

#include <moducom/libusb.h>
#include <service.libusb/acm.h>

#include <moducom/libusb/service.hpp>
#include <moducom/libusb/diagnostic.hpp>

#include <iostream>
#include <iomanip>

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
#define ENABLE_LIVE_USB_TEST 0

void printDevices(const entt::registry& registry)
{
    std::cout.fill('0');

    registry.each([&](const entt::entity e)
    {
        const auto& device = registry.get<LibUsb::Device>(e);
        const moducom::libusb::ConfigDescriptor* _config = registry.try_get<moducom::libusb::ConfigDescriptor>(e);

        moducom::diagnostic::dump(std::cout, device, _config);
    });
}

// since this is called on the USB event "thread", we need to get in and out of here asap
void printer(moducom::libusb::Buffer buffer)
{
    auto s = std::make_unique<std::string>((const char*)buffer.buffer, buffer.length);

    // At least we get a pseudo-queue of one async process by doing this
    async_result = std::async(std::launch::async, [buffer](auto _s)
    {
        std::cout << "sz=" << buffer.length << ": ";
        std::cout << *_s;
        std::cout << std::endl;
    }, std::move(s));
}

TEST_CASE("usb")
{
    entt::registry registry;
    agents::EnttHelper eh(registry, registry.create());
    using namespace std::chrono_literals;
    agents::Aggregator services(eh);
    typedef agents::Event<LibUsb> libusb_agent_type;

    SECTION("acm")
    {
        LibUsb libusb;

        entt::entity libusb_entity = registry.create();

        //services.createService<LibUsb>();

        registry.emplace<moducom::services::Description>(libusb_entity, LibUsb::description());
        registry.emplace<LibUsb*>(libusb_entity, &libusb);

        auto devices = libusb.registry.view<LibUsb::Device>();

        auto result = std::find_if(std::begin(devices), std::end(devices), [&](auto& entity)
        {
            const auto& device = devices.get<LibUsb::Device>(entity);

            if(device.vid() == CP210x::VID &&
                device.pid() == CP210x::PID)
            {
                return true;
            }

            return false;
        });


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

        // DEBT: Gonna get limited mileage out of a null-initialized device handle
        //moducom::libusb::DeviceHandle deviceHandle(nullptr);
        //AcmLibUsb acm(deviceHandle, 0, 0);
    }
    SECTION("bulk")
    {
        using namespace std::chrono_literals;
        entt::entity libusb_entity2 = services.createService<libusb_agent_type>();

        auto& libusb2 = services.getService<libusb_agent_type>(libusb_entity2);

        libusb2.construct();

        auto& libusb = libusb2.service();

        printDevices(libusb.registry);

        entt::entity deviceEntity = libusb.findDevice([](const LibUsb::Device& d)
        {
            return
                d.vid() == CP210x::VID &&
                d.pid() == CP210x::PID;
        });

#if ENABLE_LIVE_USB_TEST
        if(deviceEntity != entt::null)
        {
            moducom::libusb::DeviceHandle dh = libusb.openDeviceHandle(deviceEntity);

            dh.set_auto_detach_kernel_driver(true);

            dh.claim_interface(0);

            {
                // THOUGHTS - UNCONFIRMED:
                // 0 = infinite timeout, so in other words, waits forever until we get full buffer
                // frustratingly, this seems to lose stuff along the way though could just be
                // artifact of our goofy semi-async printer debug method
                // On second thought, I'm thinking the "wait forever until full" isn't right.  I
                // need a real TTY that I can push characters back and forth to really test this

                // OBSERVATIONS
                // We don't get any overflows at 64, even though it likes to do 300~ size transfers
                // for this use case.
                LibUsbTransfer in(eh, dh, CP210x::inEndpoint, 64);

                //auto& sink = eh.get<entt::sink<void (moducom::libusb::Buffer)> >();

                //sink.connect<printer>();

                in.sinkTransferCompleted.connect<printer>();

                bool dmaMode = in.start();

                INFO("dmaMode=" << dmaMode);

                libusb2.run();
                libusb2.run();

                in.stop();
            }

            dh.release_interface(0);

            dh.close();
        }
#endif

        libusb2.destruct();
    }
    SECTION("Transfer semi-service")
    {
        // TODO: Revive super-synthetic test here once we sort out protected DeviceHandler constructor
        /*
        // DEBT: Gonna get limited mileage out of a null-initialized device handle
        moducom::libusb::DeviceHandle deviceHandle(nullptr);

        // DEBT: Lucky that libusb 1.0.23 doesn't crash on a null device handle here.  I'm
        // sure other scenarios will
        moducom::libusb::services::Transfer<
                moducom::libusb::services::internal::TransferEnttImpl> dummy(deviceHandle, 0, 256); */
    }
}