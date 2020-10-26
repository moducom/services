#pragma once

#include "agents.hpp"

namespace moducom { namespace services { namespace managers {

namespace internal {

class StdThreadServiceToken : public ServiceToken
{
protected:
    std::thread worker;

    StdThreadServiceToken(const stop_token& stopToken) :
        ServiceToken(stopToken)
    {}

    ~StdThreadServiceToken()
    {
        worker.detach();
    }
};

// TAgent must be a StandaloneStdThread
template <class TAgent>
class SpecializedServiceToken : public StdThreadServiceToken
{
    TAgent& agent;

public:
    SpecializedServiceToken(const stop_token& stopToken, TAgent& agent) :
        StdThreadServiceToken(stopToken),
        agent(agent)
    {

    }

    void start() override
    {
        worker = agent.run(stopToken);
    }
};

}

class StandaloneStdThreadManager : public agents::Aggregator
{
    stop_source stopSource;
    agents::EnttHelper entity;
    typedef agents::Aggregator base_type;

    std::vector<agent_type*> _agents() const
    {
        // DEBT: dual dependsOn, need to fix that
        return agents::Depender::dependsOn;
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
        internal::SpecializedServiceToken<decltype(agent)> serviceToken(stopSource.token(), agent);
        return serviceToken;
    }

    /// Be advised, this is a blocking call
    void stop()
    {
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
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(100ms);
            for (agent_type* agent : _agents())
            {
                if(agent->status() != Status::Stopped)
                    stillRunning++;
            }
        }
        while(stillRunning > 0);
    }

    ~StandaloneStdThreadManager()
    {

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

}}}