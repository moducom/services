#include <moducom/libusb.h>
#include <entt/entt.hpp>

namespace moducom { namespace services {

inline void LibUsb::add_device(libusb_device* device)
{
    libusb::Device d(device);

    entt::entity id = registry.create();

    try
    {
        libusb::ConfigDescriptor config = d.get_active_config_descriptor();

        registry.emplace<libusb::ConfigDescriptor>(id, config);
    }
    catch (const libusb::Exception& e)
    {
        // unconfigured devices are not unusual
    }

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

    registry.emplace<Device>(id, device);
}

inline void LibUsb::remove_device(entt::entity deviceId)
{
    auto config = registry.try_get<libusb::ConfigDescriptor>(deviceId);
    // FIX: Somehow, freeing this config descriptor invokes a segfault at the end of LibUsb dtor
    if(config != nullptr)
    {
        //config->free();
    }


}


inline void LibUsb::remove_device(libusb_device* device)
{
    entt::entity result = findDevice([&](const Device& d)
    {
        return *d.device == device;
    });

    if(result != entt::null)
        remove_device(result);
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

void LibUsb::init()
{
    // on macOS, enabling this indicates libusb does appear to be working.  Perhaps it auto
    // filters system devices?  No external USB devices at this time for me to test with
    //context.option(LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);

    if(libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG ))
    {
        context.hotplug_register_callback(
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
}

LibUsb::LibUsb(bool autoInit)
{
    context.init();
#ifdef __WINDOWS__
    // Guidance from
    // https://github.com/libusb/libusb/wiki/Windows#Driver_Installation
    // NOTE: Gets a LIBUSB_ERROR_NOT_FOUND unless one installs
    // https://cgit.freedesktop.org/spice/win32/usbdk
    //context.option(LIBUSB_OPTION_USE_USBDK);
#endif

    if(autoInit)
        init();
}

LibUsb::~LibUsb()
{
    if(libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG ))
        context.hotplug_deregister_callback(hotplug_callback_handle);

    auto view = registry.view<Device>();
    registry.destroy(view.begin(), view.end());

    context.exit();
}

}}