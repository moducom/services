#pragma once

#include <moducom/services/status.h>
#include <moducom/semver.h>
#include <moducom/portable_endian.h>

#include <new>
#include <moducom/stop_token.h>

namespace moducom { namespace services {

class ServiceToken
{
protected:
    const stop_token& stopToken;

    ServiceToken(const stop_token& stopToken) :
            stopToken(stopToken) {}

public:
    virtual void start() = 0;
};


class ServiceBase
{
public:
    enum class ThreadPreference
    {
        Default,
        PreferCooperative,
        PreferThreaded,
        RequireThreaded
    };

    static constexpr ThreadPreference threadPreference()
    {
        return ThreadPreference::Default;
    }
};



}}