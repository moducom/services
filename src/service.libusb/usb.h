#pragma once

#include <libusb-1.0/libusb.h>
#include "services/services.h"
#include <stdexcept>

namespace moducom {

// raw libusb C++ wrappers, nothing fancy here
namespace libusb {

class Exception : public std::exception
{
    const libusb_error error;

public:
    Exception(libusb_error error) : error(error) {}
};

class DeviceHandle
{
    libusb_device_handle* const device_handle;

public:
    DeviceHandle(libusb_device_handle* device_handle) :
            device_handle(device_handle) {}

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
        auto result = (libusb_error)libusb_release_interface(device_handle, interface_number);

        if(result != LIBUSB_SUCCESS) throw Exception(result);
    }

    void alloc_streams(uint32_t num_streams, unsigned char* endpoints, int num_endpoints)
    {
        auto result = (libusb_error) libusb_alloc_streams(device_handle, num_streams, endpoints, num_endpoints);

        if(result != LIBUSB_SUCCESS) throw Exception(result);
    }

    void free_streams(unsigned char* endpoints, int num_endpoints)
    {
        auto result = (libusb_error) libusb_free_streams(device_handle, endpoints, num_endpoints);

        if(result != LIBUSB_SUCCESS) throw Exception(result);
    }

    unsigned char* alloc(size_t length)
    {
        return libusb_dev_mem_alloc(device_handle, length);
    }

    void free(unsigned char* buffer, size_t length)
    {
        auto result = (libusb_error) libusb_dev_mem_free(device_handle, buffer, length);

        if(result != LIBUSB_SUCCESS) throw Exception(result);
    }
};

}

namespace services {

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



}}