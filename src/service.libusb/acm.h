#pragma once

#include "usb.h"

// [1] See "USB Class Definitions for Communications Devices" v1.1 (1999)
// https://cscott.net/usb_dev/data/devclass/usbcdc11.pdf

namespace moducom { namespace services {

enum class AcmRequestTypeType : uint8_t
{
    ///< DEBT: Don't know real name of this request type
    Type1 = 0x20,
};

// [1] 3.6.2.1 Table 4
enum class AcmRequestType : uint8_t
{
    SET_LINE_CODING = 0x20,
    SET_CONTROL_LINE_STATE = 0x22
};

// [1] 6.2.13 Table 50
struct
    __attribute__ ((packed))
    AcmLineCoding
{
    uint32_t dwDTERate;
    uint8_t bCharFormat;
    uint8_t bParityType;
    uint8_t bDataBits;
};

class LibUsbBidirectionalDeviceBase
{
protected:
    entt::sigh<void (const unsigned char*, int)> sighTransferReceived;

    libusb::DeviceHandle deviceHandle;

    libusb::Transfer in;
    libusb::Transfer out;
    libusb::Transfer control;
public:
    entt::sink<void (const unsigned char*, int)> sinkTransferReceived;

    LibUsbBidirectionalDeviceBase(libusb::DeviceHandle deviceHandle) :
        deviceHandle(deviceHandle),
        sinkTransferReceived{sighTransferReceived}
    {
        libusb_transfer* t = control;

        // auto free() buffers for control transfers
        t->flags |= LIBUSB_TRANSFER_FREE_BUFFER;
    }
};

class AcmLibUsb : public LibUsbBidirectionalDeviceBase
{
    typedef LibUsbBidirectionalDeviceBase base_type;

    void transferCallbackControl(libusb_transfer* transfer);
    void transferCallbackBulk(libusb_transfer* transfer);
    void transferCallback(libusb_transfer* transfer);

    static void _transferCallback(libusb_transfer* transfer);

    bool dmaBufferMode;
    int interfaceNumber;

public:
    AcmLibUsb(libusb::DeviceHandle deviceHandle, uint8_t inEndpoint, uint8_t outEndpoint,
              int interfaceNumber = 0);
    ~AcmLibUsb();

    void setLineCoding(uint32_t bps,
                       uint8_t bCharFormat = 0,
                       uint8_t bParityType = 0,
                       uint8_t bDataBits = 8)
    {
        // FIX: Doesn't free yet - but maybe the auto-free option helps us here
        //unsigned char* raw = deviceHandle.alloc(sizeof(libusb_control_setup) + sizeof(AcmLineCoding));
        auto raw = (unsigned char*) malloc(sizeof(libusb_control_setup) + sizeof(AcmLineCoding));
        auto m = (AcmLineCoding*) (raw + sizeof(libusb_control_setup));

        m->dwDTERate = htole32(bps);
        m->bCharFormat = bCharFormat;
        m->bParityType = bParityType;
        m->bDataBits = bDataBits;

        libusb_fill_control_setup(raw,
                                  (uint8_t)AcmRequestTypeType::Type1,
                                  (uint8_t)AcmRequestType::SET_LINE_CODING,
                                  0, 0, sizeof(AcmLineCoding));
        control.fill_control_transfer(deviceHandle, raw, _transferCallback, this, 1000);

        // DEBT: Need to pay attention to any errors
        out.submit();
    }

    void sendCharacter(char c)
    {
        libusb_transfer* t = out;

        if(t->status != LIBUSB_TRANSFER_COMPLETED)
        {
            return;
        }

        *t->buffer = c;

        out.submit();
    }
};

}}