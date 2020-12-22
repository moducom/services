#pragma once

#include <moducom/portable_endian.h>

#ifdef __WINDOWS__
#pragma warning(push)
// Non-standard extensions used for the 'zero buffer' approach, but I am 98% sure MSVC handles that
// OK
#pragma warning(disable: 4200)
#endif

#include <libusb-1.0/libusb.h>

#ifdef __WINDOWS__
#pragma warning(pop)
#endif

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

// We'd use span, but that's only available in C++20
struct Buffer
{
    unsigned char* buffer;
    int length;
};

// only wrapper class which auto allocs/frees itself
class TransferBase
{
protected:
    libusb_transfer* const transfer;

public:
    TransferBase(int iso_packets = 0) : transfer(libusb_alloc_transfer(iso_packets))
    {

    }

    operator libusb_transfer* const () const
    {
        return transfer;
    }

    ~TransferBase()
    {
        libusb_free_transfer(transfer);
    }

    int length() const { return transfer->length; }

    libusb_error submit()
    {
        return (libusb_error) libusb_submit_transfer(transfer);
    }

    libusb_error cancel()
    {
        return (libusb_error) libusb_cancel_transfer(transfer);
    }

    void fill_bulk_transfer(libusb_device_handle* dev_handle, unsigned char endpoint,
                            unsigned char* buffer, int length, libusb_transfer_cb_fn callback,
                            void* user_data, unsigned timeout)
    {
        libusb_fill_bulk_transfer(transfer, dev_handle, endpoint, buffer,
                                  length, callback, user_data, timeout);
    }


    void fill_control_transfer(libusb_device_handle* dev_handle,
                               unsigned char* buffer, libusb_transfer_cb_fn callback,
                               void* user_data, unsigned timeout)
    {
        libusb_fill_control_transfer(transfer, dev_handle, buffer, callback, user_data, timeout);
    }

    Buffer buffer(bool use_total_length = false)
    {
        return Buffer{transfer->buffer,
                      use_total_length ? transfer->length : transfer->actual_length};
    }
};





class DeviceHandle
{
    libusb_device_handle* const device_handle;

    friend class Device;
    friend class Transfer;

    DeviceHandle(libusb_device_handle* device_handle) :
            device_handle(device_handle) {}

public:
    DeviceHandle(DeviceHandle&&) = default;
    DeviceHandle(const DeviceHandle&) = default;

    bool valid() const { return device_handle != nullptr; }

    libusb_device* device() const
    {
        return libusb_get_device(device_handle);
    }

    operator libusb_device_handle* const() const
    {
        return device_handle;
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
        libusb_close(device_handle);
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

    /// Unicode
    /// \param desc_index
    /// \param langid
    /// \param data
    /// \param length
    /// \return
    int get_string_descriptor(uint8_t desc_index, uint8_t langid, unsigned char* data, int length)
    {
        int result = libusb_get_string_descriptor(device_handle, langid, desc_index, data, length);

        if(result > 0) return result;
        else throw Exception((libusb_error)result);
    }

    /// Gets C-style ASCII descriptor, using first language supported by device
    /// \param desc_index
    /// \param data
    /// \param length
    int get_string_descriptor(uint8_t desc_index, char* data, int length)
    {
        int result = libusb_get_string_descriptor_ascii(device_handle, desc_index,
                                                                        (unsigned char*)data, length);

        if(result > 0) return result;
        else throw Exception((libusb_error)result);
    }

#if __cplusplus > 201103L
    std::string get_string_descriptor(uint8_t desc_index)
    {
        // starting from C++11 and gaining reliability forward, we can peer right into
        // string's buf
        // https://stackoverflow.com/questions/39200665/directly-write-into-char-buffer-of-stdstring
        std::string s;

        s.resize(128);

        get_string_descriptor(desc_index, &s.front(), 128);

        return s;
    }

    std::u16string get_u16string_descriptor(uint8_t desc_index, uint16_t langid = 1)
    {
        std::u16string s;

        s.resize(128);

        // DEBT: Almost definitely an endian issue happening here
        get_string_descriptor(desc_index, langid,
                              (unsigned char*)&s.front(), 128);

        return s;
    }

#endif

    bool kernel_driver_active(int if_num)
    {
        int result = libusb_kernel_driver_active(device_handle, if_num);

        if(result == 1) return true;
        else if(result == 0) return false;
        else throw Exception((libusb_error)result);
    }

    void detach_kernel_driver(int interface_number)
    {
        auto result = (libusb_error) libusb_detach_kernel_driver(device_handle, interface_number);

        if(result != LIBUSB_SUCCESS) throw Exception(result);
    }

    /// \param inhibitException since unsupported platforms can safely ignore this call, we default
    /// to silently swallowing the LIBUSB_ERROR_NOT_SUPPORTED which emits in that case - since that's
    /// the only documented failure possibility
    void set_auto_detach_kernel_driver(bool enable, bool inhibitException = true)
    {
        auto result = (libusb_error) libusb_set_auto_detach_kernel_driver(device_handle, enable);

        if(!inhibitException && result != LIBUSB_SUCCESS) throw Exception(result);
    }

    void claim_interface(int interface_number)
    {
        auto result = (libusb_error) libusb_claim_interface(device_handle, interface_number);

        if(result != LIBUSB_SUCCESS) throw Exception(result);
    }


    void release_interface(int interface_number)
    {
        auto result = (libusb_error) libusb_release_interface(device_handle, interface_number);

        if(result != LIBUSB_SUCCESS) throw Exception(result);
    }
};


/// Probably useless
class ConfigDescriptor
{
    libusb_config_descriptor* const config;

public:
    ConfigDescriptor(libusb_config_descriptor* config) :
            config(config)
    {

    }

    ConfigDescriptor(const ConfigDescriptor&) = default;

    ConfigDescriptor(ConfigDescriptor&& moveFrom) :
        config(moveFrom.config)
    {
        // DEBT: Almost definitely should make config = null in moveFrom
    }

    ConfigDescriptor& operator=(ConfigDescriptor&& moveFrom)
    {
        new (*this) ConfigDescriptor(std::move(moveFrom));
        return *this;
    }

    operator libusb_config_descriptor*() const { return config; }

    void free()
    {
        libusb_free_config_descriptor(config);
    }
};


class Device
{
    libusb_device* const device;

public:
    Device(libusb_device* device) : device(device) {}

    operator libusb_device*() const { return device; }

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

    ConfigDescriptor get_active_config_descriptor()
    {
        libusb_config_descriptor* config;

        auto status = (libusb_error) libusb_get_active_config_descriptor(device, &config);

        if(status != LIBUSB_SUCCESS) throw Exception(status);

        return config;
    }

    ConfigDescriptor get_config_descriptor(int index)
    {
        libusb_config_descriptor* config;

        auto status = (libusb_error) libusb_get_config_descriptor(device, index, &config);

        if(status != LIBUSB_SUCCESS) throw Exception(status);

        return config;
    }

    void get_device_descriptor(libusb_device_descriptor* descriptor)
    {
        auto status = (libusb_error) libusb_get_device_descriptor(device, descriptor);

        if(status != LIBUSB_SUCCESS) throw Exception(status);
    }
};


class Context
{
    libusb_context* context;

public:
    void init()
    {
        auto error = (libusb_error)libusb_init(&context);

        if(error) throw libusb::Exception(error);
    }

    void exit()
    {
        libusb_exit(context);
    }

    bool event_handling_ok()
    {
        int status = libusb_event_handling_ok(context);

        if(status == 1) return true;
        else if(status == 0) return false;
        else throw libusb::Exception((libusb_error)status);
    }

    bool event_handler_active()
    {
        int status = libusb_event_handler_active(context);

        if(status == 1) return true;
        else if(status == 0) return false;
        else throw libusb::Exception((libusb_error)status);
    }

    void lock_events()
    {
        libusb_lock_events(context);
    }

    void unlock_events()
    {
        libusb_unlock_events(context);
    }

    void handle_events()
    {
        auto status = (libusb_error) libusb_handle_events(context);

        if(status != LIBUSB_SUCCESS) throw libusb::Exception(status);
    }

    void handle_events(timeval* tv, int* completed = nullptr)
    {
        auto status = (libusb_error)
                libusb_handle_events_timeout_completed(context, tv, completed);

        if(status != LIBUSB_SUCCESS) throw libusb::Exception(status);
    }

    void hotplug_register_callback(libusb_hotplug_event events,
                                   libusb_hotplug_flag flags,
                                   int vendor_id, int product_id,
                                   int dev_class,
                                   libusb_hotplug_callback_fn cb_fn,
                                   void *user_data,
                                   libusb_hotplug_callback_handle *callback_handle)
    {
        auto status = (libusb_error) libusb_hotplug_register_callback(context,
                  events, flags, vendor_id, product_id, dev_class, cb_fn,
                  user_data, callback_handle);

        if(status != LIBUSB_SUCCESS) throw libusb::Exception(status);
    }

    void hotplug_deregister_callback(libusb_hotplug_callback_handle callback_handle)
    {
        libusb_hotplug_deregister_callback(context, callback_handle);
    }

    operator libusb_context*() const { return context; }

    template <class ...TArgs>
    void option(libusb_option option, TArgs&&...args)
    {
        auto status = (libusb_error)libusb_set_option(context, option,
                                                      std::forward<TArgs>(args)...);

        if (status != LIBUSB_SUCCESS) throw libusb::Exception(status);
    }
};


class Transfer : public TransferBase
{
public:
    Transfer(int iso_packets = 0) : TransferBase(iso_packets)
    {

    }

    DeviceHandle device_handle() const
    {
        return transfer->dev_handle;
    }
};



}}