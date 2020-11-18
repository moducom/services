#include "catch2/catch.hpp"
#include "services.h"

#include <service.libusb/usb.h>
#include <service.libusb/acm.h>

using namespace moducom::services;


TEST_CASE("usb")
{
    LibUsb libusb;
    // DEBT: Gonna get limited mileage out of a null-initialized device handle
    moducom::libusb::DeviceHandle deviceHandle(nullptr);

    SECTION("acm")
    {
        AcmLibUsb acm(deviceHandle);
    }
}