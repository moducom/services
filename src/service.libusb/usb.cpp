#include "usb.h"

namespace moducom { namespace services {


// NOTE: Can't truly do a refresh here yet because we have to free device list too
inline void LibUsb::refresh_devices()
{
    device_list_count = libusb_get_device_list(context, &device_list);
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