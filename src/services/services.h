#pragma once

#include <moducom/services/status.h>
#include <moducom/semver.h>
#include <moducom/portable_endian.h>

#include <new>
#include <moducom/stop_token.h>
#include "moducom/services/token.h"

namespace moducom { namespace services {

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
