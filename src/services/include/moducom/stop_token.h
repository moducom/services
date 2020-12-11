#pragma once

#include <atomic>

#define FEATURE_MC_SERVICES_ENTT_STOPTOKEN 1

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
    stop_source& stop_source_;

    stop_token(stop_source& source) :
        stop_source_(source) {}
#else
    //bool stop_requested_ = false;
    std::atomic<bool> stop_requested_ = false;
#endif

    friend class stop_source;

#if !FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    template <class T>
#endif
    friend class stop_callback;

public:
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
    std::atomic<bool> stop_requested_ = false;
    entt::sigh<void ()> sigh_stop_requested;

#if !FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    template <class T>
#endif
    friend class stop_callback;

public:
    const bool stop_requested() const noexcept
    {
        return stop_requested_;
    }

    bool request_stop() noexcept
    {
        stop_requested_ = true;
        sigh_stop_requested.publish();
        return true;
    }

    // DEBT: Should be a const, but non-const needed for sigh to work
    [[nodiscard]] stop_token token() noexcept
    {
        return stop_token(*this);
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
    return stop_source_.stop_requested();
}
#endif

#if !FEATURE_MC_SERVICES_ENTT_STOPTOKEN
template <class Callback>
#endif
class stop_callback
{
#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    entt::sink<void ()> sink_stop_requested;
#endif

public:
#if !FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    typedef Callback callback_type;
#endif

#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    // FIX: Not ready for primetime
    template <auto C>
    stop_callback(stop_token st) :
        sink_stop_requested{st.stop_source_.sigh_stop_requested}
    {
        sink_stop_requested.connect<C>();
    }
#endif
};

}}