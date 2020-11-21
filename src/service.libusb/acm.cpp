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

inline void LibUsbTransferIn::transferCallbackBulk(libusb_transfer* transfer)
{
    sighTransferReceived.publish(libusb::Buffer{
            transfer->buffer,
            transfer->actual_length});

    in.submit();
}


inline void LibUsbTransferIn::transferCallbackCancel(libusb_transfer* transfer)
{
    if(dmaBufferMode)
        in.device_handle().free(transfer->buffer, in.length());
    else
        free(transfer->buffer);

    status(Status::Stopped);
}



void LibUsbTransferIn::_transferCallback(libusb_transfer* transfer)
{
    if(transfer->status == LIBUSB_TRANSFER_CANCELLED)
        ((LibUsbTransferIn*)transfer->user_data)->transferCallbackCancel(transfer);
    else
        ((LibUsbTransferIn*)transfer->user_data)->transferCallbackBulk(transfer);
}

}}