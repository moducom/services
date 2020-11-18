#include "acm.h"

namespace moducom { namespace services {

void AcmLibUsb::transferCallbackBulk(libusb_transfer* transfer)
{
    // DEBT: Have to do this first because we only are using one buffer at the moment
    sighTransferReceived.publish(transfer->buffer, transfer->actual_length);

    std::string test((char*)transfer->buffer);

    if(transfer == in)
    {
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


AcmLibUsb::AcmLibUsb(libusb::DeviceHandle deviceHandle) :
    base_type(deviceHandle)
{
    // FIX: Hardcoded for CP210x
    uint8_t inEndpoint = 0x82;
    uint8_t outEndpoint = 0x01;

    // DEBT: These need to be adjustable
    uint16_t inSize = 64;
    int interface_number = 0;

    // DEBT: only invalid in unit test scenarios
    if(deviceHandle.valid())
    {
        deviceHandle.claim_interface(interface_number);

        // TODO: Optimization/experimentation - see if we can have a "large" and "small"
        // transfer block greater than 1 character

        // set up bulk transfers for in and out.  Leaving buffer as null as we
        // play some games to assign that manually below
        out.fill_bulk_transfer(deviceHandle, outEndpoint, nullptr, 1,
                               _transferCallback, this, 1000);

        in.fill_bulk_transfer(deviceHandle, inEndpoint, nullptr, inSize,
                              _transferCallback, this, 60000);

        libusb_transfer* t = out;

        t->buffer = deviceHandle.alloc(1);

        if(dmaBufferMode = (t->buffer != nullptr))
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
        int interface_number = 0;

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

        deviceHandle.release(interface_number);
    }
}

}}