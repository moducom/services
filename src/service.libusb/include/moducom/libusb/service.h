#pragma once

#include "../../../wrapper.h"
#include <entt/signal/sigh.hpp>
#include <bitset>

// doing this instead of moducom::services::libusb so as to not confuse elsewhere's
// libusb::Transfer and friends
namespace moducom { namespace libusb { namespace services {

// Useful for repetitive transfers
class Transfer
{
    libusb::Transfer transfer;

    friend class TransferAgent;

    entt::sigh<void (libusb::Transfer&)> sighCompleted;
    entt::sigh<void (libusb::Transfer&)> sighStatus;

    void callback();
    static void transferCallback(libusb_transfer* transfer);

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

    void start(unsigned char* external = nullptr);
    void stop()
    {
        transfer.cancel();
    }
};

}}}