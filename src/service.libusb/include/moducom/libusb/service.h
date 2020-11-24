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
        OneShot
    };

    std::bitset<4> flags;

public:
    Transfer(libusb::DeviceHandle deviceHandle,
             uint8_t endpoint, int length, unsigned timeout = 0)
    {
        transfer.fill_bulk_transfer(deviceHandle, endpoint, nullptr, length,
                                    transferCallback, this, timeout);

    }

    // TBD: For one shots
    void restart() {}

    void start();
    void stop()
    {
        transfer.cancel();
    }
};

}}}