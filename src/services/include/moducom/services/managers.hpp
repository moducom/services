#pragma once

#include "../../../services.h"
#include "../../../agents.hpp"

// DEBT: Need to be gentler about this, even though I personally won't be using the global
// min macro others might
#undef min

namespace moducom { namespace services { namespace managers {

namespace internal {

class StdThreadServiceToken :
    public moducom::services::internal::ServiceSignalingStopToken
{
    typedef ServiceSignalingStopToken base_type;
protected:
    std::thread worker;

    StdThreadServiceToken(const stop_token& stopToken) :
        base_type(stopToken)
    {}

    StdThreadServiceToken(StdThreadServiceToken&& moveFrom) :
        base_type(moveFrom.stopToken),
        worker(std::move(moveFrom.worker))
    {
    }

    ~StdThreadServiceToken()
    {
        if(worker.joinable())
            worker.detach();
    }
};

// TAgent must be a StandaloneStdThread
template <class TAgent>
class SpecializedServiceToken : public StdThreadServiceToken
{
    TAgent& agent_;

public:
    typedef TAgent agent_type;

    SpecializedServiceToken(const stop_token& stopToken, TAgent& agent) :
        StdThreadServiceToken(stopToken),
        agent_(agent)
    {

    }

    SpecializedServiceToken(SpecializedServiceToken&& moveFrom) :
        StdThreadServiceToken(std::move(moveFrom)),
        agent_(moveFrom.agent_)
    {

    }

    void start() override
    {
        worker = agent_.run(stopToken);
    }

    TAgent& agent() const { return agent_; }
};


template <class TAgent>
class EventToken : public ServiceToken
{
    TAgent& agent_;

    typedef typename TAgent::service_type service_type;
    // DEBT: Need this, and need it for constructor not 'run'
    //typedef typename moducom::internal::ArgType<decltype(&service_type::run)>::tuple_type event_args;

public:
    // DEBT: args ignored at this time
    template <class ...TArgs>
    EventToken(TAgent& agent, TArgs&&...args) :
        agent_(agent)
    {

    }

    void start() override
    {
        agent_.construct();
    }

    void stop() override
    {
        agent_.destruct();
    }
};


template <class TAgent, class ...TArgs>
class EventTokenExp : public ServiceToken
{
    TAgent agent_;

    typedef typename TAgent::service_type service_type;
    typedef std::tuple<TArgs...> event_args;

    event_args args_;

    inline void checkStatusAndStop()
    {
        Status status = agent_.status();

        if(status != Status::Stopping &&
            status != Status::Stopped)
        {
            agent_.destruct();
        }
    }

public:
    // DEBT: args ignored at this time
    EventTokenExp(moducom::services::agents::EnttHelper eh, TArgs&&...args) :
            agent_(eh),
            args_(std::forward<TArgs>(args)...)
    {

    }

    ~EventTokenExp()
    {
        checkStatusAndStop();
    }

    void start() override
    {
        std::apply([&](const TArgs&... args)
                   {
                       agent_.construct(args...);
                   }, args_);

        //agent_.construct();
    }

    void stop() override
    {
        checkStatusAndStop();
    }
};

}

class StandaloneStdThreadManager : public agents::Aggregator
{
    stop_source stopSource;
    typedef agents::Aggregator base_type;

    std::vector<agent_type*> _agents() const
    {
        // DEBT: dual dependsOn, need to fix that
        return agents::Depender::dependsOn();
    }

public:
    typedef agents::Aggregator::agent_type agent_type;

    /// Creates a new service of type TService for this manager to track,
    /// does *not* autostart it
    /// \tparam TService
    /// \tparam TArgs
    /// \param args
    template <class TService, class ...TArgs>
    auto push(TArgs&&...args)
    {
        agents::EnttHelper e(entity.registry, entity.registry.create());
        auto agent = new agents::StandaloneStdThread<TService, TArgs...>(e,
                std::forward<TArgs&&>(args)...);
        base_type::add(*agent);
        internal::SpecializedServiceToken<decltype(*agent)> serviceToken(stopSource.token(), *agent);
        return serviceToken;
    }


    template <class TService, class ...TArgs>
    void push_and_start(TArgs&&...args)
    {
        agents::EnttHelper e(entity.registry, entity.registry.create());
        auto agent = new agents::StandaloneStdThread<TService, TArgs...>(e,
             std::forward<TArgs&&>(args)...);
        base_type::add(*agent);
        // TODO: Incomplete, this is going to crash since thread ownership gets tossed
        // to the wind
        agent->run(stopSource.token());
    }

    /// Be advised, this is a blocking call
    void stop(std::chrono::milliseconds timeout = std::chrono::milliseconds(2000))
    {
        using namespace std::chrono_literals;
        constexpr auto slice = 100ms;

        stopSource.request_stop();

        // Might be enough to merely set stop signal
        for(agent_type* agent : _agents())
        {
            //agent->stop();
        }

        // DEBT: One single slow spinwait isn't too bad, but still not best practice
        // DEBT: Would be nice to use event/sink instead of recounting every time
        int stillRunning = 0;
        do
        {
            std::this_thread::sleep_for(slice);
            for (agent_type* agent : _agents())
            {
                Status status = agent->status();

                if(status != Status::Stopped &&
                    status != Status::Unstarted
                    )
                    stillRunning++;
            }

            timeout -= slice;
        }
        while(stillRunning > 0 && timeout > slice.zero());
    }

    StandaloneStdThreadManager(agents::EnttHelper eh) :
        agents::Aggregator(eh)
    {}

    ~StandaloneStdThreadManager()
    {
        // DEBT: Clear out dependencies -- works, but is nonintuitive the way this reads
        base_type::clear();

        for(agent_type* agent : _agents())
        {
            // DEBT: Watch out, since we set 'stopped' just *before* agent thread truly
            // shuts down, it's possible to hit an invalid state (not a race condition really)
            // and std::thread may throw an exception of improper shutdown
            if(agent->status() != Status::Stopped)
            {
                // TODO: force agent's thread into detached mode and log an error that
                // service couldn't shut down properly
            }

            // NOTE: thread ownership is currently held by SpecializedStartToken, which currently
            // isn't owned by the agent, but perhaps should be

            delete agent;
        }
    }
};

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

class EventManager : public agents::Aggregator
{
    typedef agents::Aggregator base_type;

public:
    EventManager(EnttHelper eh) : base_type(eh)
    {

    }

    template <class TService, class ...TArgs>
    auto push(TArgs&&...args)
    {
        agents::EnttHelper e(entity.registry, entity.registry.create());
        typedef agents::Event<TService> agent_type;
        auto agent = new agent_type(e, std::forward<TArgs>(args)...);
        base_type::add(*agent);
        internal::EventToken<agent_type> serviceToken(*agent);
        return serviceToken;
    }

    // experimenting with holding on to token in registry, and having token actually
    // be container for agent as well
    template <class TService, class ...TArgs>
    auto push_exp(TArgs&&...args)
    {
        agents::EnttHelper e(entity.registry, entity.registry.create());
        typedef agents::Event<TService> agent_type;
        //auto agent = new agent_type(e, std::forward<TArgs>(args)...);
        //base_type::add(*agent);
        auto* serviceToken = new internal::EventTokenExp<agent_type, TArgs...>(
                e, std::forward<TArgs>(args)...);
        e.registry.emplace<std::unique_ptr<ServiceToken> >(e.entity, serviceToken);
        return serviceToken;
    }


    void stop()
    {
        for(Agent* agent : agents::Depender::dependsOn())
        {
            // TODO: no convenient way to issue agent service's destructor yet
            // - I'm thinking stop_callback might be an interesting way to do
            // it
        }
    }
};

}}}
