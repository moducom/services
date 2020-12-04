#pragma once

#include "../stop_token.h"

namespace moducom { namespace services {

class ServiceToken
{
public:
    virtual ~ServiceToken() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
};

namespace internal {

class ServiceSignalingStopToken : public ServiceToken
{
protected:
    const stop_token& stopToken;

    ServiceSignalingStopToken(const stop_token& stopToken) :
            stopToken(stopToken) {}

    // DEBT: Not ready yet, need "multi token" ability for this
    void stop() override {}
};

}

}}
