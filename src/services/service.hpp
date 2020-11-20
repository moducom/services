#pragma once

#include <moducom/stop_token.h>
#include "services.h"

#include <entt/signal/delegate.hpp>
#include <entt/signal/sigh.hpp>
#include <entt/entt.hpp>

#include <algorithm>
#include <thread>
#include <tuple>
#include <utility>

namespace moducom { namespace services {

class Manager
{

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

#include "managers.hpp"