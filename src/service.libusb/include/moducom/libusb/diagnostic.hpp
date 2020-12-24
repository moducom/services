#pragma once

// DEBT: We do this to get at LibUsb::Device - but really LibUsb::Device should be a standalone
// class
#include "../libusb.h"
#include <iomanip>

namespace moducom { namespace diagnostic {

// DEBT: Almost definitely better to make this a .cpp, tossing in .hpp just to get it done
inline void dump(std::ostream& out,
                 const services::LibUsb::Device& device,
                 const moducom::libusb::ConfigDescriptor* _config)
{
    out << "Device: ";

    out << std::hex;
    out << std::setw(4);
    out << device.vid() << ":";
    out << std::setw(4);
    out << device.pid();

    out << " - class=" << (int)device.descriptor().bDeviceClass;
    out << ", configs=" << (int)device.descriptor().bNumConfigurations;
    out << ", addr=" << (int)device.address();
    out << ", ver=" << (int)device.releaseNumber();

    out << std::dec;
    out << std::endl;

    if (_config != nullptr)
    {
        const libusb_config_descriptor* config = *_config;

        for (int i = 0; i < config->bNumInterfaces; i++)
        {
            const libusb_interface& interface = config->interface[i];
            out << "  interface = " << (int)interface.altsetting->bInterfaceNumber << std::endl;
            out << "  class     = " << (int)interface.altsetting->bInterfaceClass << std::endl;
            out << "  subclass  = " << (int)interface.altsetting->bInterfaceSubClass << std::endl;
            out << "  protocol  = " << (int)interface.altsetting->bInterfaceProtocol << std::endl;
        }
    }
}

}}