#pragma once

#include <services/services.h>
#include <chrono>
#include "libusb/wrapper.h"
#include "libusb/scoped.h"
#include <services/agents.hpp>

#include <entt/entity/registry.hpp>

namespace moducom { namespace services {

class LibUsb : public ServiceBase
{
    typedef ServiceBase base_type;

    libusb::Context context;
    libusb_hotplug_callback_handle hotplug_callback_handle;

    void remove_device(entt::entity deviceId);
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
    LibUsb(bool autoInit = true);
    ~LibUsb();

    void init();

    struct Device
    {
        Scoped<libusb::Device> device;
        libusb_device_descriptor device_descriptor;
        //libusb::ConfigDescriptor config;

        Device(libusb::Device device) :
                device(device)
        {
            device.get_device_descriptor(&device_descriptor);
        }

        Device(Device&& moveFrom) = default;
        Device& operator=(Device&& moveFrom) = default;

        uint16_t pid() const
        {
            return le16toh(device_descriptor.idProduct);
        }

        uint16_t vid() const
        {
            return le16toh(device_descriptor.idVendor);
        }

        libusb::DeviceHandle open()
        {
            return device->open();
        }
    };


    static Description description()
    {
        return Description("libusb", SemVer{0, 1, 0});
    }

    // DEBT: Exposing this publicly seems not right somehow
    entt::registry registry;

    Device& getDevice(entt::entity entity)
    {
        return registry.get<Device>(entity);
    }

    libusb::DeviceHandle openDeviceHandle(entt::entity entity)
    {
        return getDevice(entity).device->open();
    }

    template <class F>
    entt::entity findDevice(F&& f)
    {
        auto devices = registry.view<Device>();

        auto result = std::find_if(std::begin(devices), std::end(devices), [&](entt::entity entity)
        {
            const Device& device = devices.get<Device>(entity);

            return f(device);
        });

        return result == std::end(devices) ? entt::null : *result;
    }

    // Set up to be run periodically on its own thread.
    // This flavor blocks, future ones don't have to once you overcome the nuance of
    // libusb's lock/unlock nonblocking thread stuff
    void run(moducom::services::stop_token* stopToken = nullptr)
    {
        timeval tv;

        // pop out every second
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        // NOTE: Needs to be initialized to 0, otherwise special skip
        // behavior enacts
        int completed = 0;

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