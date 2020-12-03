/**
 * @file    agent.h
 * @brief   Where base Agent class lives
 * @details Agents decouple platform/implementation specifics from a service type, as well as hold on
 *          to state data (such as status) for convenience.
 */

#pragma once

#include <entt/signal/delegate.hpp>
#include <entt/signal/sigh.hpp>
#include <entt/entt.hpp>

#include <algorithm>
#include <tuple>
#include <utility>

#include "status.h"
#include "description.h"

namespace moducom { namespace services { namespace agents {

// DEBT: EnttHelper does not belong in this file or namespace
class EnttHelper
{
public:
    entt::registry& registry;
    const entt::entity entity;

public:
    EnttHelper(entt::registry& registry, entt::entity entity) :
            registry(registry), entity(entity) {}

    EnttHelper(const EnttHelper& copyFrom) = default;

    template<class TComponent>
    TComponent& get()
    {
        return registry.get<TComponent>(entity);
    }

    template<class TComponent>
    const TComponent& get() const
    {
        return registry.get<TComponent>(entity);
    }
};

}

class Agent
{
    entt::sigh<void(Agent*, Status)> statusSignal_;
    entt::sigh<void(Agent*, Progress)> progressSignal_;
    entt::sigh<void(Agent*, Alert)> alertSignal_;

protected:
    typedef agents::EnttHelper EnttHelper;

    EnttHelper entity;

public:
    entt::sink<void(Agent*, Status)> statusSink;
    entt::sink<void(Agent*, Progress)> progressSink;
    entt::sink<void(Agent*, Alert)> alertSink;


    Agent(EnttHelper entity) :
            entity(entity),
            statusSink{statusSignal_},
            progressSink{progressSignal_},
            alertSink{alertSignal_}
    {}

    Status status_ = Status::Unstarted;

    void status(Status s)
    {
        status_ = s;
        // DEBT: Having both ECS and event style status probably gonna cause issues later, should
        // choose just one
        entity.registry.emplace_or_replace<Status>(entity.entity, s);
        statusSignal_.publish(this, s);
    }


    // DEBT: In my experience, 'message' portion really wants to be something like a std::string
    // because sometimes custom-built messages are presented
    void progress(short percentage, std::string message, const char* subsystem = nullptr)
    {
        Progress p{percentage, subsystem, message};

        progressSignal_.publish(this, p);
    }

    void progress(short percentage)
    {
        Progress p{percentage, nullptr};

        progressSignal_.publish(this, p);
    }

    void error(std::string message, const char* subsystem = nullptr)
    {
        alertSignal_.publish(this, Alert{message, subsystem, Alert::Error});
    }

public:
    Status status() const { return status_; }

    const Description& description() const
    {
        return entity.get<Description>();
    }
};


}}