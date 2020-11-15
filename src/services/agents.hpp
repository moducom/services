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

#include "status.h"

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

// DEBT: Fix bad naming
class BaseBase
{
    // TODO: probably phase out this dependsOn, managing that externally now
    std::vector<entt::entity> dependsOn;

    entt::sigh<void(BaseBase*, Status)> statusSignal_;
    entt::sigh<void(BaseBase*, Progress)> progressSignal_;
    entt::sigh<void(BaseBase*, Alert)> alertSignal_;

protected:
    EnttHelper entity;

public:
    entt::sink<void(BaseBase*, Status)> statusSink;
    entt::sink<void(BaseBase*, Progress)> progressSink;
    entt::sink<void(BaseBase*, Alert)> alertSink;


    BaseBase(EnttHelper entity) :
            entity(entity),
            statusSink{statusSignal_},
            progressSink{progressSignal_},
            alertSink{alertSignal_}
    {}

    Status status_ = Status::Unstarted;

    void status(Status s)
    {
        status_ = s;
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
    typedef BaseBase agent_type;

    std::vector<agent_type*> dependsOn;
    entt::sigh<void (bool)> signalSatisfied;
    bool satisfied_ = false;

private:
    void satisfied(bool satisfied)
    {
        if(satisfied == satisfied_) return;

        satisfied_ = satisfied;
        signalSatisfied.publish(satisfied);
    }


    void dependentStatusChanged(agent_type* agent, Status status)
    {
        bool anyNotRunning =
                std::any_of(dependsOn.begin(), dependsOn.end(), [&](BaseBase* item)
                {
                    return item->status() != Status::Running;
                });

        satisfied(anyNotRunning);
    }

public:
    entt::sink<void (bool)> sinkSatisfied;

    void add(agent_type& agent)
    {
        agent.statusSink.connect<&Depender::dependentStatusChanged>(*this);
        dependsOn.push_back(&agent);
        // Brute force a status change to update our own aggregated status
        dependentStatusChanged(&agent, agent.status());
    }

    Depender() : sinkSatisfied{signalSatisfied} {}
    ~Depender()
    {
        for(agent_type* agent : dependsOn)
        {
            agent->statusSink.disconnect<&Depender::dependentStatusChanged>(*this);
        }
    }
};


class Aggregator :
        public BaseBase,
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
        BaseBase(eh)
    {}

    void start()
    {
        status(Status::Starting);
    }

    void stop()
    {
        status(Status::Stopping);
    }
};


template <class TService>
class Base : public BaseBase
{
    std::aligned_storage_t<sizeof(TService), alignof(TService)> raw;

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

    // some guidance from
    // https://stackoverflow.com/questions/28187732/placement-new-in-stdaligned-storage
    template <class ...TArgs>
    void construct(TArgs&&... args)
    {
        //new (&service()) service_type(std::forward<TArgs>(args)...);
        ::new ((void*) ::std::addressof(raw)) service_type(std::forward<TArgs>(args)...);
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

}

template <class TService, class ...TArgs>
class AsyncEventQueue : public Base<TService>
{
    typedef Base<TService> base_type;
    typedef std::tuple<TArgs...> event_args;

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

            std::apply([&](const TArgs&... args)
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

