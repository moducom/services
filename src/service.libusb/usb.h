#pragma once

#include <services/services.h>
#include <chrono>
#include "wrapper.h"

#ifdef __GNUC__
// to get at endian conversions.  For CLang and MSVC, endian conversions appear to be
// ambiently available
#include <arpa/inet.h>
#endif

#include <entt/entity/registry.hpp>

namespace moducom {

namespace libusb {



template <class T>
struct Guard;

template <>
class Guard<DeviceHandle>
{
    DeviceHandle handle;

public:
    Guard(libusb_device* device) :
        handle(DeviceHandle::open(device))
    {
    }

    DeviceHandle* operator ->() { return &handle; }

    ~Guard()
    {
        handle.close();
    }
};

}

namespace services {

class LibUsb
{
    libusb::Context context;
    libusb_hotplug_callback_handle hotplug_callback_handle;

    void add_device(libusb_device*);
    void remove_device(libusb_device*);
    void refresh_devices();

    void hotplug_callback(libusb_context* context, libusb_device* device,
                          libusb_hotplug_event event);

    // "This callback may be called by an internal event thread and as such it is recommended the callback do minimal processing before returning."
    static int hotplug_callback(libusb_context* context, libusb_device* device,
                                 libusb_hotplug_event event, void* user_data)
    {
        ((LibUsb*)user_data)->hotplug_callback(context, device, event);
        return 0;
    }

public:
    LibUsb();
    ~LibUsb();

    // DEBT: Exposing this publicly seems not right somehow
    entt::registry registry;

    // Set up to be run periodically on its own thread
    void run(moducom::services::stop_token* stopToken = nullptr)
    {
        timeval tv;

        // pop out every second
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int completed;

        std::chrono::steady_clock c;
        auto timeout = std::chrono::seconds(5);

        std::chrono::steady_clock::time_point start = c.now();

        do
        {
            context.handle_events(&tv, &completed);
        }
        while(
                !(stopToken != nullptr && stopToken->stop_requested()) &&
                (c.now() - start) < timeout);
    }
};



}}