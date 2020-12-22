#pragma once

#include <services/services.h>
#include <chrono>
#include "libusb/wrapper.h"
#include <services/agents.hpp>

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

class LibUsb : public ServiceBase
{
    typedef ServiceBase base_type;

    libusb::Context context;
    libusb_hotplug_callback_handle hotplug_callback_handle;

    void remove_device(libusb::Device d, entt::entity deviceId);
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
        libusb::Device device;
        libusb_device_descriptor device_descriptor;
        //libusb::ConfigDescriptor config;

        Device(libusb::Device device) :
                device(device)
        {
            device.ref();

            libusb_get_device_descriptor(device, &device_descriptor);
        }

        ~Device()
        {
            device.unref();
        }

        //LibUsbMeta(const LibUsbMeta&) = default;

        Device(Device&&) = default;

        Device& operator=(const Device& copyFrom)
        {
            new (&device) libusb::Device(copyFrom.device);
            device_descriptor = copyFrom.device_descriptor;
            return *this;
        }
    };


    static Description description()
    {
        return Description("libusb", SemVer{0, 1, 0});
    }

    // DEBT: Exposing this publicly seems not right somehow
    entt::registry registry;

    libusb::Device getDevice(entt::entity entity)
    {
        return registry.get<libusb_device*>(entity);
    }

    libusb::DeviceHandle openDeviceHandle(entt::entity entity)
    {
        return getDevice(entity).open();
    }

    template <class F>
    entt::entity findDevice(F&& f)
    {
        auto desciptors = registry.view<libusb_device_descriptor>();

        auto result = std::find_if(std::begin(desciptors), std::end(desciptors), [&](entt::entity entity)
        {
            const libusb_device_descriptor& deviceDescriptor = desciptors.get<libusb_device_descriptor>(entity);

            return f(deviceDescriptor);
        });

        return result == std::end(desciptors) ? entt::null : *result;
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