#include "usb.h"
#include <entt/entt.hpp>

namespace moducom { namespace services {

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
        entt::entity id = registry.create();

        auto& device_descriptor = registry.emplace<libusb_device_descriptor>(id);

        bool correctType = device_descriptor.bDescriptorType == LIBUSB_DT_DEVICE;

        libusb_get_device_descriptor(device, &device_descriptor);

        /*
        libusb_interface_descriptor interface_descriptor;

        libusb_get_descriptor(device, LIBUSB_DT_INTERFACE, 0, &interface_descriptor,
            LIBUSB_DT_INTERFACE_SIZE); */

        uint8_t port_number = libusb_get_port_number(device);

        registry.emplace<uint8_t>(id, port_number);
    }
}

LibUsb::LibUsb()
{
    int success = libusb_init(&context);

    refresh_devices();

    libusb_hotplug_register_callback(
        context,
        LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, // | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, // "It is the user's responsibility to call libusb_close"
        LIBUSB_HOTPLUG_NO_FLAGS,
        LIBUSB_HOTPLUG_MATCH_ANY,
        LIBUSB_HOTPLUG_MATCH_ANY,
        LIBUSB_HOTPLUG_MATCH_ANY,
        &hotplug_callback,
        this,
        &hotplug_callback_handle
        );

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