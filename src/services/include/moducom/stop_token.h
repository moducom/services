#pragma once

#include <atomic>

#define FEATURE_MC_SERVICES_ENTT_STOPTOKEN 1
#define FEATURE_MC_SERVICES_ENTT_STOPTOKEN_EMITTER 0

#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
#include <entt/signal/sigh.hpp>
#include <entt/signal/emitter.hpp>
#endif

// doing this since C++20 is still so new.
// consider putting this into estdlib

namespace moducom { namespace services {

class stop_source;

class stop_token
{
#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    stop_source* stop_source_;

    stop_token(stop_source* source) :
        stop_source_(source) {}
#else
    //bool stop_requested_ = false;
    std::atomic<bool> stop_requested_ = false;
#endif

    friend class stop_source;

    template <class T>
    friend class stop_callback;

public:
#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    stop_token() : stop_source_{nullptr} {}

    stop_token(const stop_token& copy_from) :
        stop_source_(copy_from.stop_source_)
    {

    }

    stop_token(stop_token&& move_from) :
        stop_source_(move_from.stop_source_)
    {
        move_from.stop_source_ = nullptr;
    }

    [[nodiscard]] bool stop_possible() const noexcept
    {
        return stop_source_ != nullptr;
    }
#endif

    [[nodiscard]] bool stop_requested() const noexcept
#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    ;
#else
    {
        bool value = stop_requested_;
        return value;
    }
#endif
};


class stop_source
{
#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    struct event
    {
        unsigned stop_requested : 1;
        unsigned shutdown : 1;
    };
    std::atomic<bool> stop_requested_ = false;
    entt::sigh<void (event)> sigh;

    // NOTE: Need this at stop source so that when source leaves scope it can notify tokens it's gone
    // otherwise we'd have to use a shared_ptr
    entt::sink<void (event)> sink{sigh};

    template <class T>
    friend class stop_callback;

public:
    [[nodiscard]] const bool stop_requested() const noexcept
    {
        return stop_requested_;
    }

    bool request_stop() noexcept
    {
        stop_requested_ = true;
        sigh.publish(event{1, 0});
        return true;
    }

    // DEBT: Should be a const, but non-const needed for sigh to work
    [[nodiscard]] stop_token token() noexcept
    {
        return stop_token(this);
    }
#else
    stop_token token_;

public:
    const stop_token& token() const { return token_; }

    const bool stop_requested() const noexcept
    {
        return token_.stop_requested();
    }

    bool request_stop() noexcept
    {
        token_.stop_requested_ = true;
        return true;
    }
#endif
};

#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
inline bool stop_token::stop_requested() const noexcept
{
    return stop_possible() && stop_source_->stop_requested();
}
#endif

template <class Callback>
class stop_callback
{
#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    entt::connection c;
    Callback callback;

    void enttFriendlyCallback(stop_source::event e)
    {
        if(e.stop_requested)
            callback();

        // DEBT: Probably not needed, doing a connection release here, but this way
        // we're extra safe that we don't release against a no-longer-existing sink
        if(e.shutdown)
            c.release();
    }
#endif

public:
    typedef Callback callback_type;

#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    template <class C>
    stop_callback(stop_token st, C&& cb) :
        // DEBT: Unknown if this does a proper forward, but I believe it does
        callback{cb}
    {
        c = st.stop_source_->sink.connect<&stop_callback::enttFriendlyCallback>(this);
    }

    ~stop_callback()
    {
        c.release();
    }
#endif
};

#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
#ifdef __cpp_deduction_guides
template<class Callback>
stop_callback(stop_token, Callback) -> stop_callback<Callback>;
#else
// TODO: Make a make_stop_callback for C++14 compat.  EnTT only goes back to C++14
#endif
#endif


}}