#pragma once

#include "agents.hpp"

namespace moducom { namespace services { namespace managers {

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