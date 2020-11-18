#include "acm.h"

namespace moducom { namespace services {

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

void AcmLibUsb::_transferCallback(libusb_transfer* transfer)
{
    // NOTE: Expecting a LIBUSB_TRANSFER_CANCELLED here occasionally after AcmLibUsb itself
    // goes out of scope/destructs.  Though not awesome, it's OK because this is a static
    // method
    if(transfer->status == LIBUSB_TRANSFER_CANCELLED)   return;

    ((AcmLibUsb*)transfer->user_data)->transferCallback(transfer);
}

}}