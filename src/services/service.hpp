#pragma once

#include "semver.h"
#include "stop_token.h"
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
    Description(char* name, SemVer version) :
        name_(name), version_(version)
    {}
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


namespace agents {

// DEBT: Fix bad naming
class BaseBase
{

};

template <class TService>
class Base : public BaseBase
{
    std::aligned_storage<sizeof(TService), alignof(TService)> raw;

protected:
    typedef TService service_type;
    typedef service_type& reference;

    reference service()
    {
        return reinterpret_cast<reference>(raw);
    }
};

template <class TService, ServiceBase::ThreadPreference>
class SingleShot;

template <class TService>
class SingleShot<TService,
        ServiceBase::ThreadPreference::PreferThreaded> : public Base<TService>
{
    typedef Base<TService> base_type;

public:
};


template <class TService>
class Periodic : public Base<TService>
{
    typedef Base<TService> base_type;

};

}

namespace managers {

/// Handles both fixed-time and variable-time periodic interval agents
/// Relies on external mechanism to call this at the right time
template <class TTimeBase>
class Scheduler
{
    struct Item
    {
        TTimeBase wakeup;
        agents::BaseBase* agent;
    };

    std::vector<Item> agents;

    const Item& first() const { return agents.front(); }

    void sort()
    {
        std::sort_heap(agents.begin(), agents.end(),
                       [&](const Item& a, const Item& b)
                       {
                            return a.wakeup > b.wakeup;
                       });
    }
};

}

}}