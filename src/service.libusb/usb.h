#pragma once

#include <libusb-1.0/libusb.h>
#include "services/services.h"

namespace moducom { namespace services {

class LibUsb
{
    libusb_context* context;
    libusb_device** device_list;
    ssize_t device_list_count;
    libusb_hotplug_callback_handle hotplug_callback_handle;

    void refresh_devices();

    // "This callback may be called by an internal event thread and as such it is recommended the callback do minimal processing before returning."
    static int hotplug_callback(libusb_context* context, libusb_device* device,
                                 libusb_hotplug_event event, void* user_data)
    {
        return 0;
    }

public:
    LibUsb();
    ~LibUsb();
};


class LibUsbDevice
{
    libusb_device_handle* const device_handle;

public:
    LibUsbDevice(libusb_device_handle* device_handle) :
        device_handle(device_handle)
    {}

    libusb_device* device() const
    {
        return libusb_get_device(device_handle);
    }

    void claim(int interface_number)
    {
        libusb_claim_interface(device_handle, interface_number);
    }

    void release(int interface_number)
    {
        libusb_release_interface(device_handle, interface_number);
    }
};

}}