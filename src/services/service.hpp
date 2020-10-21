#pragma once

#include "semver.h"
#include "stop_token.h"

#include <algorithm>
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


class PeriodicBase
{
public:
    // relative time
    // DEBT: change names to not conflict with absolute time named the same below
    typedef int timebase_type;

    virtual timebase_type run(timebase_type passed) = 0;
};


/// Fixed-period intervals
/// \tparam TService
template <class TService>
class Periodic :
        public PeriodicBase,
        public Base<TService>
{
    typedef Base<TService> base_type;
    const timebase_type interval;

public:
    Periodic(timebase_type interval) :
        interval(interval) {}

    timebase_type run(timebase_type passed) override
    {
        base_type::service().run(passed);
        return interval;
    }
};

/// Variable relative intervals
/// \tparam TService
template <class TService>
class ScheduledRelative :
        public PeriodicBase,
        public Base<TService>
{
    typedef Base<TService> base_type;

public:
    timebase_type run(timebase_type passed) override
    {
        return base_type::service().run(passed);
    }
};

}

namespace managers {

/// Handles both fixed-time and variable-time periodic interval agents
/// Relies on external mechanism to call this at the right time
template <class TTimeBase>
class Scheduler
{
    // absolute time
    typedef TTimeBase timebase_type;

    struct Item
    {
        TTimeBase wakeup;
        agents::PeriodicBase* agent;
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

public:
    void add(agents::PeriodicBase* agent)
    {
        Item item {10, agent};
        agents.emplace_back(item);
    }

    void run()
    {
        first().agent->run(1);
    }
};

}

}}