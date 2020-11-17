#include "usb.h"
#include <entt/entt.hpp>

namespace moducom { namespace services {

inline void LibUsb::add_device(libusb_device* device)
{
    entt::entity id = registry.create();

    auto& device_descriptor = registry.emplace<libusb_device_descriptor>(id);

    bool correctType = device_descriptor.bDescriptorType == LIBUSB_DT_DEVICE;

    libusb_get_device_descriptor(device, &device_descriptor);

    registry.emplace<libusb_device*>(id, device);

    uint8_t port_number = libusb_get_port_number(device);

    registry.emplace<uint8_t>(id, port_number);
}

// NOTE: Can't truly do a refresh here yet because we have to free device list too
inline void LibUsb::refresh_devices()
{
    // DEBT: Unlikely we really want to fully clear this -
    // - for main init, not necessary
    // - for updates, a merge would be preferable
    registry.clear();

    device_list_count = libusb_get_device_list(context, &device_list);

    for (int i = 0; i < device_list_count; i++)
    {
        libusb_device* device = device_list[i];

        add_device(device);

        /*
        libusb::Guard<libusb::DeviceHandle> handle(device);

        unsigned char* buf = handle->alloc(10);
        handle->free(buf, 10); */

        /*
        libusb_interface_descriptor interface_descriptor;

        libusb_get_descriptor(device, LIBUSB_DT_INTERFACE, 0, &interface_descriptor,
            LIBUSB_DT_INTERFACE_SIZE); */
    }
}

void LibUsb::hotplug_callback(libusb_context* context, libusb_device* device,
                      libusb_hotplug_event event)
{
    add_device(device);
}

LibUsb::LibUsb()
{
    auto error = (libusb_error)libusb_init(&context);

    if(error) throw libusb::Exception(error);

    // TOOD: Need to populate out into registry rather than hold on to device_list
    //if(libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG ))
    if(false)
    {
        libusb_hotplug_register_callback(
                context,
                LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, // | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, // "It is the user's responsibility to call libusb_close"
                LIBUSB_HOTPLUG_ENUMERATE,
                LIBUSB_HOTPLUG_MATCH_ANY,
                LIBUSB_HOTPLUG_MATCH_ANY,
                LIBUSB_HOTPLUG_MATCH_ANY,
                &hotplug_callback,
                this,
                &hotplug_callback_handle
        );
    }
    else
    {
        refresh_devices();
    }

    //libusb_device_descriptor d;

    //success = libusb_get_device_descriptor(device_list[0], &d);
}

LibUsb::~LibUsb()
{
    libusb_hotplug_deregister_callback(context, hotplug_callback_handle);
    libusb_free_device_list(device_list, 1);
    libusb_exit(context);
}

}}