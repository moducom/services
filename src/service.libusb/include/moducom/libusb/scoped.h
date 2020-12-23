#pragma once

#include <moducom/scoped.h>
#include "wrapper.h"

namespace moducom {

template <>
class Scoped<libusb::DeviceHandle>
{
    typedef libusb::DeviceHandle element_type;

    element_type handle;

public:
    Scoped(libusb_device* device) :
            handle(element_type::open(device))
    {
    }

    element_type* operator ->() { return &handle; }
    element_type& operator *() { return handle; }

    ~Scoped()
    {
        handle.close();
    }
};

template <>
class Scoped<libusb::Device>
{
    typedef libusb::Device element_type;

    element_type device;

public:
    Scoped(libusb_device* device) :
            device(device)
    {
        this->device.ref();
    }

    Scoped(Scoped&& moveFrom) :
            device(std::move(moveFrom.device))
    {
        // since libusb::Device doesn't autoref, we do it manually -
        // this way Scoped dtor which now runs twice (moved copy + original)
        // will have device ref'd twice too to match
        this->device.ref();
    }

    Scoped& operator=(Scoped&& moveFrom)
    {
        // if move copying over an existing device, be sure to unref since it won't be pointed
        // to when 'this' dtor runs
        device.unref();
        new (this) Scoped(std::move(moveFrom));
        return *this;
    }

    ~Scoped()
    {
        this->device.unref();
    }

    element_type* operator ->() { return &device; }
    element_type& operator *() { return device; }
};


template <>
class Scoped<libusb::Transfer>
{
    typedef libusb::Transfer element_type;

    libusb::Transfer transfer;

public:
    // Temporary - eventually transfer will be a thinner wrapper and we won't
    // pass iso_packets in but rather the allocated transfer
    Scoped(int iso_packets = 0) :
            transfer(libusb::Transfer::alloc(iso_packets))
    {

    }

    ~Scoped()
    {
        //transfer.free();
    }

    libusb::Transfer& operator *() { return transfer; }
};


}