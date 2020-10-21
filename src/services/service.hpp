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


template <class TDuration>
class PeriodicBase
{
public:
    // relative time
    typedef TDuration duration_type;

    virtual duration_type run(duration_type passed) = 0;
};


/// Fixed-period intervals
/// \tparam TService
template <class TService>
class Periodic :
        public PeriodicBase<typename TService::duration_type>,
        public Base<TService>
{
    typedef Base<TService> base_type;
    typedef typename TService::duration_type duration_type;
    const duration_type interval;

public:
    Periodic(duration_type interval) :
        interval(interval) {}

    duration_type run(duration_type passed) override
    {
        base_type::service().run(passed);
        return interval;
    }
};

/// Variable relative intervals
/// \tparam TService
template <class TService>
class ScheduledRelative :
        public PeriodicBase<typename TService::duration_type>,
        public Base<TService>
{
    typedef Base<TService> base_type;
    typedef typename TService::duration_type duration_type;

public:
    duration_type run(duration_type passed) override
    {
        return base_type::service().run(passed);
    }
};

}

namespace managers {

/// Handles both fixed-time and variable-time periodic interval agents
/// Relies on external mechanism to call this at the right time
/// TODO: Consider making the whole thing follow the system_clock (and friends)
/// pattern instead, which includes duration and time_point
template <class TTimeBase, class TDuration>
class Scheduler
{
    // absolute time
    typedef TTimeBase timebase_type;
    typedef TDuration duration_type;
    typedef agents::PeriodicBase<duration_type> agent_type;

    /// lower # = higher priority, service first
    typedef unsigned short priority_type;

    struct Item
    {
        timebase_type wakeup;
        agent_type* agent;
        priority_type priority;

        bool operator < (const Item& compareTo) const
        {
            // TODO: include priority as part of this comparison
            return wakeup < compareTo.wakeup;
        }
    };

    std::vector<Item> agents;

    Item& first() { return agents.front(); }

    void sort()
    {
        std::sort_heap(agents.begin(), agents.end());
    }

public:
    Scheduler()
    {
        //std::make_heap(agents.begin(), agents.end());
    }

    void add(agent_type* agent)
    {
        Item item {10, agent, 0};
        agents.emplace_back(item);
        std::push_heap(agents.begin(), agents.end());
    }

    void _run(Item& first)
    {
        // FIX: Not passing in right 'passed' interval just yet
        duration_type wakeup_interval = first.agent->run(1);
        std::pop_heap(agents.begin(), agents.end());
        first.wakeup += wakeup_interval;
        std::push_heap(agents.begin(), agents.end());
    }

    bool run(timebase_type absolute)
    {
        Item& first = this->first();

        if(first.wakeup >= absolute)
        {
            _run(first);
            return true;
        }

        return false;
    }
};

}

}}