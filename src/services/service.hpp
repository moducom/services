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

enum class Status
{
    Unstarted,
    WaitingOnDependency,
    Starting,
    Started,
    Running,
    Pausing,
    Paused,
    Stopping,
    Stopped,

    Error
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

class EnttHelper
{
public:
    entt::registry& registry;
    const entt::entity entity;

public:
    EnttHelper(entt::registry& registry, entt::entity entity) :
        registry(registry), entity(entity)
    {}

    template <class TComponent>
    TComponent& get()
    {
        return registry.get<TComponent>(entity);
    }

    template <class TComponent>
    const TComponent& get() const
    {
        return registry.get<TComponent>(entity);
    }
};

// DEBT: Fix bad naming
class BaseBase
{
    std::vector<entt::entity> dependsOn;
    EnttHelper entity;

public:
    entt::sigh<void(Status)> statusSignal_;
    entt::sink<void(Status)> statusSink;


    BaseBase(EnttHelper entity) :
        entity(entity),
        statusSink{statusSignal_}
    {}

    Status status_ = Status::Unstarted;

    void status(Status s)
    {
        status_ = s;
        entity.registry.emplace_or_replace<Status>(entity.entity, s);
        statusSignal_.publish(s);
    }

public:
    int dependenciesRunningCount() const
    {
        int count = 0;

        for(entt::entity id : dependsOn)
        {
            auto status = entity.registry.get<Status>(id);

            if(status == Status::Running) count++;
        }

        return count;
    }

    bool dependenciesAllRunning() const
    {
        return dependenciesRunningCount() == dependsOn.size();
    }


    Status status() const { return status_; }

    const Description& description() const
    {
        return entity.get<Description>();
    }

    void dependencyStatusUpdated(entt::registry&, entt::entity id)
    {
        // TODO: Optimize and consider the particular entity rather than re-evaluating
        // them all each time for every entity status update
        bool allRunning = dependenciesAllRunning();
        if(allRunning)
        {
            if(status() == Status::WaitingOnDependency)
                status(Status::Starting);
        }
    }

    void addDependency(entt::entity id)
    {
        dependsOn.push_back(id);
        entity.registry.on_update<Status>().connect<&BaseBase::dependencyStatusUpdated>(this);
    }

    void _start()
    {
        if(!dependenciesAllRunning())
            status(Status::WaitingOnDependency);
    }
};

template <class TService>
class Base : public BaseBase
{
    std::aligned_storage<sizeof(TService), alignof(TService)> raw;

protected:
    typedef TService service_type;
    typedef service_type& reference;

    // DEBT: Eventually needs to be protected/friend access.  Making them public
    // for ease of testing/bringup
public:
    Base(EnttHelper entity) : BaseBase(entity) {}

    reference service()
    {
        return reinterpret_cast<reference>(raw);
    }

    template <class ...TArgs>
    void construct(TArgs&&... args)
    {
        new (&service()) service_type(std::forward<TArgs>(args)...);
    }

    void destruct()
    {
        service().~service_type();
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


/// Fixed-period intervals, and run indefinitely until system shuts them down
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
    Periodic(EnttHelper enttHelper, duration_type interval) :
        base_type(enttHelper),
        interval(interval) {}

    duration_type run(duration_type passed) override
    {
        base_type::service().run(passed);
        return interval;
    }
};

/// Variable relative intervals, and can request self-shutdown
/// \tparam TService
template <class TService>
class ScheduledRelative :
        public PeriodicBase<typename TService::duration_type>,
        public Base<TService>
{
    typedef Base<TService> base_type;
    typedef typename TService::duration_type duration_type;

public:
    ScheduledRelative(EnttHelper entity) : base_type(entity) {}

    duration_type run(duration_type passed) override
    {
        return base_type::service().run(passed);
    }
};


// NOTE: Not a fan of holding on to initiatization arguments forever.  So this is
// a tracer code style approach for now
template <class TService, class ...TArgs>
class StandaloneStdThread : public Base<TService>
{
    typedef Base<TService> base_type;
    typedef StandaloneStdThread this_type;

    // holding on to ctor args here
    // guidance from https://stackoverflow.com/questions/15537817/c-how-to-store-a-parameter-pack-as-a-variable
    std::tuple<TArgs...> ctor_args;

protected:
    void worker(const stop_token& stopToken)
    {
        construct();
        base_type::status(Status::Started);
        base_type::status(Status::Running);

        while(!stopToken.stop_requested())
        {
            base_type::service().run();
        }

        base_type::status(Status::Stopping);
        base_type::destruct();

        // DEBT: Would be better if this was set when thread itself terminated
        base_type::status(Status::Stopped);
    }

public:
    StandaloneStdThread(EnttHelper entity, TArgs&&... args) :
        base_type(entity),
        ctor_args(std::forward<TArgs>(args)...)
    {

    }

    void construct()
    {
        std::apply([&](const TArgs&... args)
        {
            base_type::construct(args...);
        }, ctor_args);
    }

    std::thread run(const stop_token& token)
    {
        base_type::status(Status::Starting);
        std::thread thread(&this_type::worker, this, std::ref(token));
        return thread;
    }
};

template <class TService, class ...TArgs>
auto make_standalone(EnttHelper enttHelper, TArgs&&... args)
{
    StandaloneStdThread<TService, TArgs...> agent(
            enttHelper,
            std::forward<TArgs>(args)...);
    return agent;
}

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

    enum class priorities : priority_type
    {
        highest = 0,
        high = 100,
        medium = 200,
        low = 300,
        idle = 1000
    };

    struct Item
    {
        timebase_type wakeup;
        agent_type* agent;
        priority_type priority;

        bool operator < (const Item& compareTo) const
        {
            // remember, since heaps put largest first, and we want smallest first,
            // we reverse the compare operation
            return wakeup >= compareTo.wakeup && priority >= compareTo.priority;
        }
    };

    std::vector<Item> agents;

    Item& first() { return agents.front(); }

    void sort()
    {
        std::sort_heap(agents.begin(), agents.end());
    }

    void pop()
    {
        std::pop_heap(agents.begin(), agents.end());
        agents.pop_back();
    }

public:
    Scheduler()
    {
        //std::make_heap(agents.begin(), agents.end());
    }

    const Item& cfirst() const { return agents.front(); }

    size_t count() const noexcept { return agents.size(); }

    void add(agent_type* agent, timebase_type initial_wakeup)
    {
        Item item {initial_wakeup, agent,
                   (priority_type) priorities::idle};
        agents.emplace_back(item);
        std::push_heap(agents.begin(), agents.end());
    }

    void _run(Item& first, duration_type passed)
    {
        // FIX: Not passing in right 'passed' interval just yet
        duration_type wakeup_interval = first.agent->run(passed);
        if(wakeup_interval == duration_type::min())
        {
            // This signals a removal
            pop();
            return;
        }
        first.wakeup += wakeup_interval;
        // put 'first' at the end
        std::pop_heap(agents.begin(), agents.end());
        // do heap magic to place next 'lowest' (largest technically by std::heap
        // definitions) at first
        std::push_heap(agents.begin(), agents.end());
    }

    ///
    /// \param absolute
    /// \return true when item serviced
    bool run(timebase_type absolute)
    {
        Item& first = this->first();

        if(absolute >= first.wakeup)
        {
            duration_type delta = absolute - first.wakeup;
            // FIX: delta is not right here, it has to be delta + past requested
            // interval for 'passed' to be correct
            _run(first, delta);
            return true;
        }

        return false;
    }
};

}

}}