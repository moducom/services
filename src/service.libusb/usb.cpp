#include "usb.h"
#include <entt/entt.hpp>

namespace moducom { namespace services {

inline void LibUsb::add_device(libusb_device* device)
{
    libusb::Device d(device);

    entt::entity id = registry.create();

    auto& device_descriptor = registry.emplace<libusb_device_descriptor>(id);

    libusb_get_device_descriptor(device, &device_descriptor);

    bool correctType = device_descriptor.bDescriptorType == LIBUSB_DT_DEVICE;

    d.ref();
    registry.emplace<libusb_device*>(id, device);

    uint8_t port_number = libusb_get_port_number(device);

    registry.emplace<uint8_t>(id, port_number);

    {
        // Gets LIBUSB_ERROR_ACCESS
        //libusb::Guard<libusb::DeviceHandle> handle(device);

        /*
        unsigned char* buf = handle->alloc(10);
        handle->free(buf, 10); */

        /*
        libusb_interface_descriptor interface_descriptor;

        libusb_get_descriptor(device, LIBUSB_DT_INTERFACE, 0, &interface_descriptor,
            LIBUSB_DT_INTERFACE_SIZE); */
    }
}


inline void LibUsb::remove_device(libusb_device* device)
{
    libusb::Device d(device);

    auto devices = registry.view<libusb_device*>();

    auto result = std::find_if(std::begin(devices), std::end(devices), [&](entt::entity entity)
    {
        return devices.get(entity) == device;
    });

    if(result != std::end(devices))
    {
        entt::entity found = *result;
        d.unref();
        registry.remove_all(found);
    }
}

// NOTE: Can't truly do a refresh here yet because we have to free device list too
inline void LibUsb::refresh_devices()
{
    libusb_device** device_list;
    ssize_t device_list_count;

    // DEBT: Unlikely we really want to fully clear this -
    // - for main init, not necessary
    // - for updates, a merge would be preferable
    registry.clear();

    device_list_count = libusb_get_device_list(context, &device_list);

    for (int i = 0; i < device_list_count; i++)
    {
        libusb_device* device = device_list[i];

        add_device(device);

    }

    libusb_free_device_list(device_list, 1);
}

void LibUsb::hotplug_callback(libusb_context* context, libusb_device* device,
                      libusb_hotplug_event event)
{
    switch(event)
    {
        case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED:
            add_device(device);
            break;

        case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT:
            remove_device(device);
            break;
    }
}

// NOTE: may be phasing out this intensive use of registry awareness *inside* agents.  Not sure
// yet
static entt::registry dummyRegistry;

LibUsb::LibUsb() :
        base_type(agents::EnttHelper(dummyRegistry, dummyRegistry.create()))
{
    status(Status::Starting);

    context.init();

    if(libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG ))
    {
        libusb_hotplug_register_callback(
                context,
                LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, // | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, // "It is the user's responsibility to call libusb_close"
                LIBUSB_HOTPLUG_ENUMERATE,
                LIBUSB_HOTPLUG_MATCH_ANY,
                LIBUSB_HOTPLUG_MATCH_ANY,
                LIBUSB_HOTPLUG_MATCH_ANY,
                hotplug_callback,
                this,
                &hotplug_callback_handle
        );
    }
    else
    {
        refresh_devices();
    }

    // Hotplug is async, so started/running doesn't necessarily mean all the devices are there yet.
    status(Status::Started);
    status(Status::Running);
}

LibUsb::~LibUsb()
{
    if(libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG ))
        context.hotplug_deregister_callback(hotplug_callback_handle);

    context.exit();
    status(Status::Stopped);
}

}}