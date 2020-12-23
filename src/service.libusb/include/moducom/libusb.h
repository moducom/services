#pragma once

#include <services/services.h>
#include <chrono>
#include "libusb/wrapper.h"
#include <services/agents.hpp>

#include <entt/entity/registry.hpp>

namespace moducom {


template <class T>
struct Scoped;

template <>
class Scoped<libusb::DeviceHandle>
{
    typedef libusb::DeviceHandle element_type;

    element_type handle;

public:
    Scoped(libusb_device* device) :
        handle(element_type::open(device))
    {
    }

    element_type* operator ->() { return &handle; }
    element_type& operator *() { return handle; }

    ~Scoped()
    {
        handle.close();
    }
};

template <>
class Scoped<libusb::Device>
{
    typedef libusb::Device element_type;

    element_type device;

public:
    Scoped(libusb_device* device) :
        device(device)
    {
        this->device.ref();
    }

    Scoped(Scoped&& moveFrom) :
        device(std::move(moveFrom.device))
    {
        // since libusb::Device doesn't autoref, we do it manually -
        // this way Scoped dtor which now runs twice (moved copy + original)
        // will have device ref'd twice too to match
        this->device.ref();
    }

    Scoped& operator=(Scoped&& moveFrom)
    {
        // if move copying over an existing device, be sure to unref since it won't be pointed
        // to when 'this' dtor runs
        device.unref();
        new (this) Scoped(std::move(moveFrom));
        return *this;
    }

    ~Scoped()
    {
        this->device.unref();
    }

    element_type* operator ->() { return &device; }
    element_type& operator *() { return device; }
};


template <>
class Scoped<libusb::Transfer>
{
    typedef libusb::Transfer element_type;

    libusb::Transfer transfer;

public:
    // Temporary - eventually transfer will be a thinner wrapper and we won't
    // pass iso_packets in but rather the allocated transfer
    Scoped(int iso_packets = 0) :
        transfer(libusb::Transfer::alloc(iso_packets))
    {

    }

    ~Scoped()
    {
        //transfer.free();
    }

    libusb::Transfer& operator *() { return transfer; }
};

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