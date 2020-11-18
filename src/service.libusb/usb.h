#pragma once

#include "services/services.h"
#include "wrapper.h"

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
    libusb_context* context;
    libusb_hotplug_callback_handle hotplug_callback_handle;

    entt::registry registry;

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
};



}}