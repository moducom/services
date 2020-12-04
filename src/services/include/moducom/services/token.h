#pragma once

#include "../stop_token.h"

namespace moducom { namespace services {

class ServiceToken
{
protected:
    const stop_token& stopToken;

    ServiceToken(const stop_token& stopToken) :
            stopToken(stopToken) {}

public:
    virtual ~ServiceToken() = default;

    virtual void start() = 0;
};

}}
