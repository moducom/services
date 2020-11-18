#include "acm.h"

namespace moducom { namespace services {

inline void AcmLibUsb::transferCallback(libusb_transfer* transfer)
{
    if(transfer->status != LIBUSB_TRANSFER_COMPLETED)
    {
        // TODO: Better error handling
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
    ((AcmLibUsb*)transfer->user_data)->transferCallback(transfer);
}

}}