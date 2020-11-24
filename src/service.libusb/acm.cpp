#include "acm.h"

namespace moducom { namespace services {

void AcmLibUsb::transferCallbackBulk(libusb_transfer* transfer)
{
    if(transfer == in)
    {
        // DEBT: Have to do this first because we only are using one buffer at the moment
        sighTransferReceived.publish(libusb::Buffer{
            transfer->buffer,
            transfer->actual_length});

        in.submit();
    }
}


void AcmLibUsb::transferCallbackControl(libusb_transfer* transfer)
{
    auto control_setup = (libusb_control_setup*) transfer->buffer;

    switch((AcmRequestTypeType)control_setup->bmRequestType)
    {
        case AcmRequestTypeType::Type1:
            switch((AcmRequestType)control_setup->bRequest)
            {
                case AcmRequestType::SET_CONTROL_LINE_STATE:
                    break;
            }
            break;
    }
}


inline void AcmLibUsb::transferCallback(libusb_transfer* transfer)
{
    if(transfer->status != LIBUSB_TRANSFER_COMPLETED)
    {
        // TODO: Better error handling
        // NOTE: In transfer doesn't have time outs at the moment, so we don't
        // expect to get here
        if(transfer->status == LIBUSB_TRANSFER_TIMED_OUT)
        {
            // We expect input to timeout frequently as we wait for something to appear,
            // so merely re-queue
            if(transfer == in)
            {
                // DEBT: 90% sure we can resubmit right here in the callback.  Not 100% sure.
                in.submit();
            }
        }

        return;
    }

    switch(transfer->type)
    {
        case LIBUSB_TRANSFER_TYPE_BULK:
            transferCallbackBulk(transfer);
            break;

        case LIBUSB_TRANSFER_TYPE_CONTROL:
            transferCallbackControl(transfer);
            break;
    }

}

void AcmLibUsb::_transferCallback(libusb_transfer* transfer)
{
    // NOTE: Expecting a LIBUSB_TRANSFER_CANCELLED here occasionally after AcmLibUsb itself
    // goes out of scope/destructs.  Though not awesome, it's OK because this is a static
    // method
    if(transfer->status == LIBUSB_TRANSFER_CANCELLED)   return;

    ((AcmLibUsb*)transfer->user_data)->transferCallback(transfer);
}


AcmLibUsb::AcmLibUsb(libusb::DeviceHandle deviceHandle, uint8_t inEndpoint, uint8_t outEndpoint,
                     int interfaceNumber) :
    base_type(deviceHandle),
    interfaceNumber(interfaceNumber)
{
    // DEBT: These need to be adjustable
    uint16_t inSize = 64;

    // DEBT: only invalid in unit test scenarios
    if(deviceHandle.valid())
    {
        deviceHandle.claim_interface(interfaceNumber);

        // TODO: Optimization/experimentation - see if we can have a "large" and "small"
        // transfer block greater than 1 character

        // set up bulk transfers for in and out.  Leaving buffer as null as we
        // play some games to assign that manually below
        out.fill_bulk_transfer(deviceHandle, outEndpoint, nullptr, 1,
                               _transferCallback, this, 1000);

        in.fill_bulk_transfer(deviceHandle, inEndpoint, nullptr, inSize,
                              _transferCallback, this, 0);

        libusb_transfer* t = out;

        t->buffer = deviceHandle.alloc(1);

        if((dmaBufferMode = (t->buffer != nullptr)))
        {
            t = in;

            t->buffer = deviceHandle.alloc(inSize);
        }
        else
        {
            t->buffer = (unsigned char*) malloc(1);

            t = in;

            t->buffer = (unsigned char*) malloc(inSize);
        }

        // kick off listening
        libusb_error error = in.submit();

        if(error != LIBUSB_SUCCESS) throw libusb::Exception(error);
    }
}


AcmLibUsb::~AcmLibUsb()
{
    if(deviceHandle.valid())
    {
        // DEBT: These must be adjustable
        uint16_t inSize = 64;

        in.cancel();
        out.cancel();

        libusb_transfer* t = out;

        if(dmaBufferMode)
        {
            deviceHandle.free(t->buffer, 1);

            t = in;

            deviceHandle.free(t->buffer, inSize);
        }
        else
        {
            free(t->buffer);

            t = in;

            free(t->buffer);
        }

        deviceHandle.release_interface(interfaceNumber);
    }
}


inline void LibUsbTransfer::transferCallbackBulk(libusb_transfer* transfer)
{
    // DEBT: Dummy functions just for debugging for now
    if(transfer->status == LIBUSB_TRANSFER_TIMED_OUT)
    {
        int l = transfer->actual_length;
        this->transfer.submit();
        return;
    }
    else if(transfer->status == LIBUSB_TRANSFER_OVERFLOW)
    {
        int l = transfer->actual_length;
        this->transfer.submit();
        return;
    }

    sighTransferCompleted.publish(libusb::Buffer{
            transfer->buffer,
            transfer->actual_length});

    this->transfer.submit();
}


inline void LibUsbTransfer::transferCallbackCancel(libusb_transfer* transfer)
{
    if(dmaBufferMode)
        this->transfer.device_handle().free(transfer->buffer, this->transfer.length());
    else
        free(transfer->buffer);

    status(Status::Stopped);
}



void LibUsbTransfer::_transferCallback(libusb_transfer* transfer)
{
    if(transfer->status == LIBUSB_TRANSFER_CANCELLED)
        ((LibUsbTransfer*)transfer->user_data)->transferCallbackCancel(transfer);
    else
        ((LibUsbTransfer*)transfer->user_data)->transferCallbackBulk(transfer);
}

}}

namespace moducom { namespace libusb { namespace services {

inline void Transfer::alloc()
{
    libusb_transfer* t = transfer;

    t->buffer = transfer.device_handle().alloc(transfer.length());

    bool dmaBufferMode = flags[DmaMode] = t->buffer != nullptr;

    if (!dmaBufferMode)
        t->buffer = (unsigned char*) malloc(transfer.length());
}

void Transfer::free()
{
    libusb_transfer* t = transfer;

    if(flags[DmaMode])
        transfer.device_handle().free(t->buffer, transfer.length());
    else if(!flags[External])
        ::free(t->buffer);
}

inline void Transfer::callback()
{
    libusb_transfer* t = transfer;

    switch(t->status)
    {
        case LIBUSB_TRANSFER_COMPLETED:
            sighCompleted.publish(transfer);
            if(!flags[OneShot])
                // DEBT: Need to check return value of this and do something if it fails
                transfer.submit();
            break;

        case LIBUSB_TRANSFER_CANCELLED:
            free();

        default:
            sighStatus.publish(transfer);
            break;
    }
}

void Transfer::transferCallback(libusb_transfer* transfer)
{
    auto _this = (Transfer*)transfer->user_data;

    // TODO: Assert that incoming 'transfer' is same as _this->transfer
    _this->callback();
}


void Transfer::start(unsigned char* external)
{
    libusb_transfer* t = transfer;

    if(external == nullptr)
        alloc();
    else
    {
        flags[External] = true;
        t->buffer = external;
    }

    libusb_error error = transfer.submit();

    if(error != LIBUSB_SUCCESS) throw libusb::Exception(error);
}

}}}