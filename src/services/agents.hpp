/**
 * @file    agents.hpp
 * @brief   Where stock implementation-specific agents live
 * @details Agents decouple platform/implementation specifics from a service type, as well as hold on
 *          to state data (such as status) for convenience.
 */

#pragma once

#include <entt/signal/delegate.hpp>
#include <entt/signal/sigh.hpp>
#include <entt/entt.hpp>

#include <algorithm>
#include <future>
#include <queue>
#include <thread>
#include <tuple>
#include <utility>

#include <moducom/services/status.h>
#include "services.h"

namespace moducom { namespace services { namespace agents {

class EnttHelper
{
public:
    entt::registry& registry;
    const entt::entity entity;

public:
    EnttHelper(entt::registry& registry, entt::entity entity) :
            registry(registry), entity(entity)
    {}

    EnttHelper(const EnttHelper& copyFrom) = default;

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

class Agent
{
    entt::sigh<void(Agent*, Status)> statusSignal_;
    entt::sigh<void(Agent*, Progress)> progressSignal_;
    entt::sigh<void(Agent*, Alert)> alertSignal_;

protected:
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

template <class T>
class SignalingVector : public std::vector<T>
{
    typedef std::vector<T> base_type;

    entt::sigh<void (T)> signalAdded;
public:
    entt::sink<void (T)> sinkAdded;

    SignalingVector() : sinkAdded{signalAdded}
    {

    }

    template <class ...TArgs>
    void push_back(TArgs&&...args)
    {
        base_type::push_back(std::forward<TArgs&&>(args)...);
    }
};

// DEBT: Split this out into .h/.cpp flavor to reduce clutter in .hpp
class Depender
{
protected:
    typedef Agent agent_type;

    std::vector<agent_type*> dependsOn_;
    entt::sigh<void (bool)> signalSatisfied;
    bool satisfied_ = false;

public:
    bool anyNotRunning() const
    {
        return std::any_of(dependsOn_.begin(), dependsOn_.end(), [](Agent* item)
                {
                    return !is_running(item->status());
                });
    }

    bool allRunning() const { return !anyNotRunning(); }

    bool satisfied() const { return satisfied_; }

private:
    void satisfied(bool satisfied)
    {
        if(satisfied == satisfied_) return;

        satisfied_ = satisfied;
        signalSatisfied.publish(satisfied);
    }


    void dependentStatusChanged(agent_type* agent, Status status)
    {
        satisfied(allRunning());
    }

public:
    entt::sink<void (bool)> sinkSatisfied;

    void clear()
    {
        for (agent_type* agent : dependsOn_)
        {
            agent->statusSink.disconnect<&Depender::dependentStatusChanged>(*this);
        }

        dependsOn_.clear();
    }

    void add(agent_type& agent)
    {
        agent.statusSink.connect<&Depender::dependentStatusChanged>(*this);
        dependsOn_.push_back(&agent);
        // Brute force a status change to update our own aggregated status
        dependentStatusChanged(&agent, agent.status());
    }

    const std::vector<agent_type*>& dependsOn() const { return dependsOn_; }

    Depender() : sinkSatisfied{signalSatisfied} {}
    ~Depender()
    {
        clear();
    }
};


class Aggregator :
        public Agent,
        // TODO: Decouple this and make Depender 1:1 with entity since both Aggregator (root taxonomy)
        // and regular agent can both have dependencies
        public Depender
{
    void dependentStatus()
    {

    }

    void dependentProgress()
    {

    }

public:
    Aggregator(EnttHelper eh) :
        Agent(eh)
    {}

    // DEBT: Resolve this with depender's vector
    // DEBT: Exposing this as publis is a no-no
    entt::registry registry;

    void start()
    {
        status(Status::Starting);
    }

    void stop()
    {
        status(Status::Stopping);
    }

    template <class TService, class ...TArgs>
    entt::entity createService(TArgs&&...args)
    {
        entt::entity entity = registry.create();
        EnttHelper eh(registry, entity);
        auto service = new TService(eh, std::forward<TArgs>(args)...);

        registry.emplace<Description>(entity, TService::description());
        registry.emplace<std::unique_ptr<TService> >(entity, service);

        // FIX: Experimental, having this and unique ptr at same time
        registry.emplace<Agent*>(entity, service);

        return entity;
    }

    template <class TService>
    TService& getService(entt::entity entity)
    {
        return *registry.get<std::unique_ptr<TService> >(entity).get();
    }
};

// NOTE: Much like estd's value_evaporator and friends
template <class TContained>
class Container
{
    std::aligned_storage_t<sizeof(TContained), alignof(TContained)> raw;

protected:
    typedef TContained contained_type;
    typedef contained_type& reference;

public:
    reference contained()
    {
        return reinterpret_cast<reference>(raw);
    }

    // some guidance from
    // https://stackoverflow.com/questions/28187732/placement-new-in-stdaligned-storage
    template <class ...TArgs>
    void construct(TArgs&&... args)
    {
        //new (&services()) service_type(std::forward<TArgs>(args)...);
        ::new ((void*) ::std::addressof(raw)) contained_type(std::forward<TArgs>(args)...);
    }

    void destruct()
    {
        contained().~contained_type();
    }
};


template <class TService>
class Base :
        public Agent,
        // DEBT: Eventually needs to be protected/friend access.  Making them public
        // for ease of testing/bringup
        public Container<TService>
{
    typedef Container<TService> container_base;

protected:
    typedef TService service_type;
    typedef service_type& reference;

public:
    Base(EnttHelper entity) : Agent(entity) {}

    reference service()
    {
        return container_base::contained();
    }

    static Description description()
    {
        return service_type::description();
    }

    template <class ... TArgs>
    void construct(TArgs&&...args)
    {
        Agent::status(Status::Starting);
        container_base::construct(std::forward<TArgs>(args)...);
        Agent::status(Status::Started);
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


template <class TService>
class Worker : public Base<TService>
{
    typedef Base<TService> base_type;

protected:
    Worker(EnttHelper entity) : base_type(entity) {}

protected:

    void worker_after_construction(const stop_token& stopToken)
    {
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
};


// NOTE: Not a fan of holding on to initiatization arguments forever.  So this is
// a tracer code style approach for now
template <class TService, class ...TArgs>
class StandaloneStdThread : public Worker<TService>
{
    typedef Worker<TService> base_type;
    typedef StandaloneStdThread this_type;

    // holding on to ctor args here
    // guidance from https://stackoverflow.com/questions/15537817/c-how-to-store-a-parameter-pack-as-a-variable
    std::tuple<TArgs...> ctor_args;

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

    void worker(const stop_token& token)
    {
        // DEBT: Would prefer the pass-function-as-template-parameter flavor and
        // completely consolidate construct() call down into Worker class.  Proved to
        // be kinda tricky, so not doing so just yet
        construct();
        base_type::worker_after_construction(token);
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


// non-async external event responder.  Be sure to handle things quickly!
template <class TService>
class Event : public Base<TService>
{
    typedef TService service_type;
    typedef Base<TService> base_type;

    // TODO: Consolidate with AsyncEvent/AsyncEventQueue
    // TODO: Revisit whether we always want to do progress 0-100 per iteration, probably not
    template <class ...TArgs>
    void runner(TArgs&&...args)
    {
        base_type::status(Status::Running);
        base_type::progress(0);
        service_type& service = base_type::service();
        service.run(std::forward<TArgs>(args)...);
        base_type::progress(100);
        base_type::status(Status::Waiting);
    }

public:
    Event(EnttHelper eh) : base_type(eh)
    {

    }
    template <class ... TArgs>
    void construct(TArgs&&...args)
    {
        base_type::construct(std::forward<TArgs>(args)...);
        Agent::status(Status::Waiting);
    }

    // remember, run is a gentle misnomer here, it really means "handle event of given args"
    template <class ...TArgs>
    void run(TArgs&&...args)
    {
        runner(std::forward<TArgs>(args)...);
    }
};

// uses std::async to run event on a different thread
template <class TService>
class AsyncEvent : public Base<TService>
{
    typedef TService service_type;
    typedef Base<TService> base_type;
    typedef AsyncEvent<TService> this_type;

public:

    template <class ...TArgs>
    void runner(TArgs&&...args)
    {
        base_type::progress(0);
        service_type& service = base_type::service();
        service.run(std::forward<TArgs>(args)...);
        base_type::progress(100);
    }

public:
    AsyncEvent(EnttHelper eh) :
        base_type(eh)
    {}

    // Nifty, but not as much of a fire-and-forget as one might like.  Remember,
    // std::future blocks on destruction (C++14)
    template <class ...TArgs>
    [[nodiscard]] auto run(TArgs&&...args)
    {
        return std::async(std::launch::async,
                &this_type::runner<TArgs...>, this,
                std::forward<TArgs>(args)...);
    }
};

namespace internal {

template <class T>
class BlockingQueue
{
    typedef T value_type;
    typedef value_type& reference;
    std::queue<T> queue;

    std::condition_variable cv;
    std::mutex cv_m;

public:
    std::queue<T>& q() { return queue; }

    std::unique_lock<std::mutex> unique_lock()
    {
        return std::unique_lock<std::mutex>(cv_m);
    }

    std::lock_guard<std::mutex> lock_guard()
    {
        return std::lock_guard<std::mutex>(cv_m);
    }

    void wait_for_presence()
    {
        std::unique_lock<std::mutex> lk(cv_m);
        cv.wait(lk, [&] {return !queue.empty();});
    }

    // DEBT: Can probably do a check for a stop_token instead, somehow
    template <class Rep, class Period>
    bool wait_for_presence(const std::chrono::duration<Rep, Period>& timeout)
    {
        std::unique_lock<std::mutex> lk(cv_m);
        return cv.wait_for(lk, timeout, [&] {return !queue.empty();});
    }


    void push(const value_type& value)
    {
        {
            std::lock_guard<std::mutex> lk(cv_m);
            queue.push(value);
        }
        cv.notify_all();
    }

    template <class ...TArgs>
    void emplace(TArgs&&...args)
    {
        {
            std::lock_guard<std::mutex> lk(cv_m);
            queue.emplace(std::forward<TArgs>(args)...);
        }
        cv.notify_all();
    }
};

// guidance from
// https://stackoverflow.com/questions/22632236/how-is-possible-to-deduce-function-argument-type-in-c
// https://stackoverflow.com/questions/29906242/c-deduce-member-function-parameters
template <class F> struct ArgType;

template <class C, class F> struct ClassArgType;

template <class R, class T>
struct ArgType<R(*)(T)>
{
    //typedef TArgs... type;
    typedef std::tuple<T> tuple_type;
};

/*
template <class C, class R, class T>
struct ArgType<R(C::*)(T)>
{
    //typedef TArgs... type;
    typedef std::tuple<T> tuple_type;
}; */

template <class C, class R, class ...TArgs>
struct ArgType<R(C::*)(TArgs...)>
{
    //typedef TArgs... type;
    typedef std::tuple<TArgs...> tuple_type;

    template <typename F>
    static R invoke(F&& f, const tuple_type& tuple)
    {
        // DEBT: TArgs... should be a ref, const ref or similar
        std::apply([&](TArgs... args)
                   {
                       f(std::forward<TArgs>(args)...);
                   }, tuple);

    }
};


/*
template <class R, class ...TArgs>
struct ArgType<R(*)(TArgs...)>
{
    //typedef TArgs... type;
    typedef std::tuple<TArgs...> tuple_type;
};*/

}

template <class TService> //, class ...TArgs>
class AsyncEventQueue : public Base<TService>
{
    typedef Base<TService> base_type;
    // NOTE: This deduction demands one and only one 'run' method be present
    typedef typename internal::ArgType<decltype(&TService::run)>::tuple_type event_args;
    //typedef std::tuple<TArgs...> event_args;

    struct Item
    {
        bool stop_signal;
        event_args args;

        Item() {}

        Item(bool stop_signal, event_args args = event_args()) :
            stop_signal(stop_signal), args(args)
        {}
    };

    internal::BlockingQueue<Item> queue;

    // DEBT: I think we can do this with just the future variable
    bool workerRunning = false;
    std::mutex workerRunningMutex;

    ///
    /// \param stopToken
    /// @details Does a semi-spin wait (500ms per iteration) so as to check for 'sleep' and stop
    /// condition
    void worker(const stop_token& stopToken)
    {
        using namespace std::chrono_literals;
        int counter = 10;

        // Worker is always started with at least one entry in the queue, so we
        // use a do/while

        workerRunningMutex.lock();

        do
        {
            workerRunningMutex.unlock();

            Item item;

            {
                auto lock = queue.lock_guard();

                item = queue.q().front();
                queue.q().pop();
            }

            if(item.stop_signal)    break;

            // DEBT: Don't like auto here, but getting tuple's TArgs is quite difficult
            std::apply([&](const auto&... args)
            {
                base_type::service().run(args...);
            }, item.args);

            workerRunningMutex.lock();
        }
        while(!stopToken.stop_requested() &&
            counter-- > 0 &&
            queue.wait_for_presence(500ms));

        workerRunning = false;

        workerRunningMutex.unlock();
    }

    // DEBT: Bring in stop_token& from outside
    stop_source stopSource;
    std::future<void> workerFuture;

public:
    AsyncEventQueue(EnttHelper eh) : base_type(eh) {}
    ~AsyncEventQueue()
    {
        stop();
        {
            auto lg = queue.lock_guard();
            while(!queue.q().empty())
            {
                Item& item = queue.q().front();
                if(!item.stop_signal)
                {
                    // TODO: note non-stop-signal lingering events
                }
                queue.q().pop();
            }
        }
    }

    void stop()
    {
        stopSource.stop_requested();
        queue.emplace(true);
    }

    /// wait for worker thread to complete
    void wait() const
    {
        workerFuture.wait();
    }

    // DEBT: Need to mate this to tuple rather than a new ...TArgs,
    // though indirectly it is since the queue.emplace will fail compilation
    // if things don't match
    template <class ...TArgs>
    void run(TArgs&&...args)
    {
        queue.emplace(false, event_args(std::forward<TArgs>(args)...));

        {
            std::unique_lock<std::mutex> ul(workerRunningMutex);

            // If worker is already running, return
            // FIX: Still have race condition where worker could be at very end of processing and
            // block its 'workerRunning = false' right when we get here.  This would result in
            // us returning early and never re-starting the worker
            if(workerRunning)   return;

            workerRunning = true;

            // future destructor will block, if necessary while worker finishes up
            workerFuture = std::async(std::launch::async, &AsyncEventQueue::worker, this,
                                      std::ref(stopSource.token()));
        }
    }
};

}}}

