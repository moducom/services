#pragma once

#include <libusb-1.0/libusb.h>
#include "services/services.h"
#include <stdexcept>

#include <entt/entity/registry.hpp>

namespace moducom {

// raw libusb C++ wrappers, nothing fancy here
namespace libusb {

class Exception : public std::exception
{
    const libusb_error error;

public:
    Exception(libusb_error error) : error(error) {}
};

class Transfer
{
    libusb_transfer* const transfer;

public:
    Transfer(int iso_packets) : transfer(libusb_alloc_transfer(iso_packets))
    {

    }

    ~Transfer()
    {
        libusb_free_transfer(transfer);
    }

    libusb_error submit()
    {
        return (libusb_error) libusb_submit_transfer(transfer);
    }

    libusb_error cancel()
    {
        return (libusb_error) libusb_cancel_transfer(transfer);
    }
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

    void control_transfer(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                          unsigned char* data, uint16_t wLength, unsigned int timeout)
    {
        auto result = (libusb_error) libusb_control_transfer(device_handle, bmRequestType, bRequest, wValue,
                                                             wIndex, data, wLength, timeout);

        if(result != LIBUSB_SUCCESS) throw Exception(result);
    }

    void bulk_transfer(unsigned char endpoint, unsigned char* data, int length,
                       int* transferred, unsigned timeout)
    {
        auto result = (libusb_error) libusb_bulk_transfer(device_handle, endpoint, data, length,
                                                          transferred, timeout);

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

    entt::registry registry;

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