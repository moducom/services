#include "acm.h"

namespace moducom { namespace services {

inline void AcmLibUsb::transferCallback(libusb_transfer* transfer)
{
}

void AcmLibUsb::_transferCallback(libusb_transfer* transfer)
{
    ((AcmLibUsb*)transfer->user_data)->transferCallback(transfer);
}

}}