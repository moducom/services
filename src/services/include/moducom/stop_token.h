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

namespace internal {

struct stop_state
{
    entt::sigh<void ()> sigh_stop_requested;
    std::atomic<bool> stop_requested_ = false;
    stop_source* owner;
};

}

class stop_token
{
    using stop_state = internal::stop_state;

#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    std::weak_ptr<stop_state> stop_state_;

    stop_token(stop_source& source);
#else
    //bool stop_requested_ = false;
    std::atomic<bool> stop_requested_ = false;
#endif

    friend class stop_source;

    template <class T>
    friend class stop_callback;

public:
#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    stop_token() = default;

    stop_token(const stop_token& copy_from) :
        stop_state_(copy_from.stop_state_)
    {

    }

    stop_token(stop_token&& move_from) :
        stop_state_(std::move(move_from.stop_state_))
    {
    }

    [[nodiscard]] bool stop_possible() const noexcept;
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
    using stop_state = internal::stop_state;

#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    std::shared_ptr<stop_state> stop_state_;

    template <class T>
    friend class stop_callback;

    friend class stop_token;

public:
    stop_source() :
        stop_state_(new stop_state())
    {
        stop_state_->owner = this;
    }

    ~stop_source()
    {
        stop_state_->owner = nullptr;
    }

    [[nodiscard]] const bool stop_requested() const noexcept
    {
        return stop_state_->stop_requested_;
    }

    bool request_stop() noexcept
    {
        stop_state_->stop_requested_ = true;
        stop_state_->sigh_stop_requested.publish();
        return true;
    }

    // DEBT: Should be a const, but non-const needed for sigh to work
    [[nodiscard]] stop_token token() noexcept
    {
        return stop_token(*this);
    }

    // Reflects that edge cases where a stop_source *without* stop state that suddenly
    // gets one later is not fully - since our current approach can't easily indicate
    // to stop_tokens that's the case.  We'd have to allocate stop_state with an empty
    // owner for that.  Not too hard really
    bool stop_possible() const noexcept { return true; }
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
inline stop_token::stop_token(stop_source& source) :
    stop_state_(source.stop_state_)
{

}


inline bool stop_token::stop_possible() const noexcept
{
    if (stop_state_.expired()) return false;

    // DEBT: Not MT safe
    std::shared_ptr<stop_state> ss = stop_state_.lock();
    stop_source* source = ss->owner;

    return source != nullptr && source->stop_possible();
}

inline bool stop_token::stop_requested() const noexcept
{
    // DEBT: Even though stop_requested_ is atomic, the .lock portion isn't so
    // we may hit MT issues here.  For that matter that && between the two isn't
    // atomic/MT safe either
    return stop_possible() && stop_state_.lock()->stop_requested_;
}
#endif

template <class Callback>
class stop_callback
{
#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    // Keeps stop_state_ alive so that disconnect has something to disconnect from.  If stop_state
    // is already gone, weak_ptr's lock behavior creates a brand new stop_state, which is sufficient
    std::weak_ptr<internal::stop_state> stop_state_;
    entt::sink<void ()> sink_stop_requested;

    Callback callback;

    void on_stop_requested()
    {
        callback();
    }
#endif

public:
    typedef Callback callback_type;

#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    template <class C>
    stop_callback(stop_token st, C&& cb) :
        stop_state_(st.stop_state_),
        // DEBT: Not MT safe
        sink_stop_requested{stop_state_.lock()->sigh_stop_requested},
        // DEBT: Unknown if this does a proper forward, but I believe it does
        callback{cb}
    {
        // DEBT: Not MT safe, stop_state_ may have gone away
        sink_stop_requested.connect<&stop_callback::on_stop_requested>(this);
    }

    ~stop_callback()
    {
        // sink_stop_requested holds a pointer to sigh_stop_requested.
        // entt:sink doesn't auto disconnect on destructor (that's what scoped_connection
        // is for)
        if(!stop_state_.expired())
            sink_stop_requested.disconnect<&stop_callback::on_stop_requested>(this);
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