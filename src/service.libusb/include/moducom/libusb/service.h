#pragma once

#include "../../../wrapper.h"
#include <entt/signal/sigh.hpp>
#include <bitset>

// doing this instead of moducom::services::libusb so as to not confuse elsewhere's
// libusb::Transfer and friends
namespace moducom { namespace libusb { namespace services {

class TransferBase
{
    // DEBT: Want this protected, but impl has a hard time seeing in in that case
public:
    libusb::Transfer transfer;

    enum Flags
    {
        DmaMode,
        OneShot,
        External
    };

    std::bitset<4> flags;

    void alloc();
    void free();

public:
    void start(unsigned char* external = nullptr);
    void stop()
    {
        transfer.cancel();
    }
};

// Useful for repetitive transfers
template <class TTransferImpl>
class Transfer : public TransferBase
{
    friend class TransferAgent;

    TTransferImpl impl;

    void callback();
    static void transferCallback(libusb_transfer* transfer);

public:
    Transfer(libusb::DeviceHandle deviceHandle,
             uint8_t endpoint, int length, unsigned timeout = 0)
    {
        transfer.fill_bulk_transfer(deviceHandle, endpoint, nullptr, length,
                                    transferCallback, this, timeout);
    }

    ///
    /// \param length
    /// \details It is assumed if (re)starting a one shot, that the previous transfer
    ///          has reached its conclusion.
    void oneshot(unsigned char* external, int length)
    {
        libusb_transfer* t = transfer;
        if(t->buffer)   free();
        t->length = length;
        start(external);
    }

};

namespace internal {

struct TransferEnttImpl
{
    entt::sigh<void (libusb::Transfer&)> sighCompleted;
    entt::sigh<void (libusb::Transfer&)> sighStatus;

public:
    entt::sink<void (libusb::Transfer&)> sinkCompleted;
    entt::sink<void (libusb::Transfer&)> sinkStatus;

    TransferEnttImpl() :
        sinkCompleted{sighCompleted},
        sinkStatus{sighStatus}
    {}

    void callback(TransferBase& parent, libusb_transfer* t);
};

}

class Device
{
protected:
    libusb::DeviceHandle deviceHandle;

public:
    Device(libusb::Device device) :
        deviceHandle(device.open())
    {
    }

    ~Device()
    {
        deviceHandle.close();
    }
};


namespace experimental {

template <class TDevice>
struct DeviceTraits;

struct CP210xTraits
{
    static constexpr uint16_t VID = 0x10c4;
    static constexpr uint16_t PID = 0xea60;
    static constexpr uint8_t inEndpoint = 0x82;
    static constexpr uint8_t outEndpoint = 0x01;

    static constexpr uint8_t endpoints[] = { outEndpoint, inEndpoint };
    static constexpr uint8_t interfaces[] = { 0 };

    static constexpr uint8_t outEndpointIdx = 0;
    static constexpr uint8_t inEndpointIdx = 1;
};

template <class TDeviceTraits>
struct Device : public services::Device
{
    typedef services::Device base_type;
    typedef TDeviceTraits traits_type;

    libusb::Transfer transfers[sizeof(traits_type::endpoints)];

    Device(libusb::Device device) :
        base_type(device)
    {
        for(uint8_t interface : traits_type::interfaces)
            deviceHandle.claim_interface(interface);
    }

    ~Device()
    {
        for(uint8_t interface : traits_type::interfaces)
            deviceHandle.release_interface(interface);
    }
};

}




}}}