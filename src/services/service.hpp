#pragma once

#include "semver.h"
#include "stop_token.h"

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


class Description
{
    const char* name_;
    const SemVer version_;

public:
    Description(const char* name, SemVer version) :
        name_(name), version_(version)
    {}

    Description(Description&& moveFrom) = default;
    Description(const Description& copyFrom) = default;

    Description& operator=(const Description& copyFrom)
    {
        new (this) Description(copyFrom);
        return *this;
    };
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