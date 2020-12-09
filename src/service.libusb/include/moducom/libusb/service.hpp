#include "service.h"

namespace moducom { namespace libusb { namespace services {

template <class TTransferImpl>
void Transfer<TTransferImpl>::transferCallback(libusb_transfer* transfer)
{
    auto _this = (Transfer*)transfer->user_data;

    // TODO: Assert that incoming 'transfer' is same as _this->transfer
    _this->impl.callback(*_this, transfer);
}



}}}