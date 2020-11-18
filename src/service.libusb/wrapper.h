#pragma once

#include <libusb-1.0/libusb.h>
#include <stdexcept>

// raw libusb C++ wrappers, nothing fancy here
namespace moducom { namespace libusb {

class Exception : public std::exception
{
public:
    const libusb_error error;

    Exception(libusb_error error) : error(error) {}

    const char* what() const noexcept override
    {
        return libusb_error_name(error);
    }
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

    DeviceHandle(DeviceHandle&&) = default;
    DeviceHandle(const DeviceHandle&) = default;

    libusb_device* device() const
    {
        return libusb_get_device(device_handle);
    }

    //  libusb_open() adds another reference which is later destroyed by libusb_close().
    static DeviceHandle open(libusb_device* device)
    {
        libusb_device_handle* dh;
        auto error = (libusb_error)libusb_open(device, &dh);
        if(error != LIBUSB_SUCCESS) throw Exception(error);
        return DeviceHandle(dh);
    }

    void close()
    {
        // FIX: Seems to lock up
        libusb_close(device_handle);
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

    void get_descriptor(uint8_t desc_type, uint8_t desc_index, unsigned char* data, int length)
    {
        auto result = (libusb_error) libusb_get_descriptor(device_handle, desc_type, desc_index, data, length);

        if(result != LIBUSB_SUCCESS) throw Exception(result);
    }
};


class Device
{
    libusb_device* const device;

public:
    Device(libusb_device* device) : device(device) {}

    Device(Device&&) = default;
    Device(const Device&) = default;

    void ref()
    {
        libusb_ref_device(device);
    }

    void unref()
    {
        libusb_unref_device(device);
    }

    DeviceHandle open()
    {
        return DeviceHandle::open(device);
    }
};

}}