#include "service.h"

namespace moducom { namespace libusb { namespace services {

template <class TTransferImpl>
void Transfer<TTransferImpl>::transferCallback(libusb_transfer* transfer)
{
    auto _this = (Transfer*)transfer->user_data;

    // TODO: Assert that incoming 'transfer' is same as _this->transfer
    if(transfer->status == LIBUSB_TRANSFER_COMPLETED)
    {
        _this->impl.onCompleted(*_this, transfer);
        if(!_this->flags[TransferBase::OneShot])
            // DEBT: Need to check return value of this and do something if it fails
            _this->transfer.submit();
    }
    else
        _this->impl.onStatus(*_this, transfer);
}



}}}