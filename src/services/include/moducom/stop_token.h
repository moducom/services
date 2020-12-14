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
    friend class linked_stop_source;

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
protected:
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


namespace internal {

class stop_callback_impl
{
    // if shared_ptr goes away weak_ptr will tell us that.  If stop_state
    // is already gone from the get go, weak_ptr's lock behavior creates a
    // brand new stop_state, which is sufficient
    std::weak_ptr<stop_state> stop_state_;

public:
    entt::sink<void ()> sink_stop_requested;

    stop_callback_impl(std::shared_ptr<stop_state>&& ss) :
            stop_state_{ss},
            sink_stop_requested{ss->sigh_stop_requested}
    {

    }

    ~stop_callback_impl()
    {
        // DEBT: Not MT safe
        if(!stop_state_.expired())
            sink_stop_requested.disconnect();
    }
};

}

template <class Callback>
class stop_callback
{
#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    using stop_state = internal::stop_state;

    typedef Callback callback_type;

    class impl : public internal::stop_callback_impl
    {
        callback_type callback;

        void on_stop_requested() const
        {
            callback();
        }

    public:
        /// Helper needed because RAII behavior of sink_stop_requested is a little
        /// unresilient to weak_ptr's maybe/maybe not personality
        /// \param ss use of shared_ptr - even temporarily - guarantees availability of
        /// underlying pointer
        impl(std::shared_ptr<stop_state>&& ss, callback_type&& callback) :
            internal::stop_callback_impl(std::move(ss)),
            // DEBT: Unknown if this does a proper forward, but I believe it does
            callback{callback}
        {
            sink_stop_requested.connect<&impl::on_stop_requested>(this);
        }
    };

    impl impl_;

#endif

public:

#if FEATURE_MC_SERVICES_ENTT_STOPTOKEN
    template <class C>
    stop_callback(stop_token st, C&& cb) :
        impl_(st.stop_state_.lock(), std::move(cb))
    {
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

// can't quite figure out syntax
// https://timmurphy.org/2014/08/28/passing-member-functions-as-template-parameters-in-c/
class linked_stop_source : public stop_source
{
    internal::stop_callback_impl impl_;

    // Cascade linked request into our own
    // Not used now, get rid of this once we prove things are working well
    void on_request_stop()
    {
        stop_source::request_stop();
    }

    //stop_callback<decltype(&linked_stop_source::handler)> callback;
    //stop_callback<lambda()&> callback;
    //stop_callback<void (*)()> callback;
    //stop_callback<void (linked_stop_source::*)()> callback;

public:
    linked_stop_source(stop_token st) :
        impl_(st.stop_state_.lock())
        //: callback(st, []{  })
        //: callback(st, &linked_stop_source::handler)
    {
        if(st.stop_requested())
        {
            stop_state_->stop_requested_ = true;
            return;
        }
        //stop_callback<void (linked_stop_source::*)(linked_stop_source*)> callback(st, &linked_stop_source::handler);

        //stop_callback test(st, [this] { this->on_request_stop(); });
        //stop_callback test2(st, [this] { this->on_request_stop(); });

        //stop_callback test(st, &linked_stop_source::handler);

        //impl_.sink_stop_requested.connect<&linked_stop_source::on_request_stop>(this);
        impl_.sink_stop_requested.connect<&linked_stop_source::request_stop>(this);
    }

    /*
     * NOTE: Consider using this if we wanna vector up our impl's to support multiple incoming tokens
    linked_stop_source(std::initializer_list<stop_token> st_list) :
            impl_(st.stop_state_.lock())
    {
        for(stop_token st : st_list)
        {
            if(st.stop_requested())
            {
                stop_state_->stop_requested_ = true;
                return;
            }

            impl_.sink_stop_requested.connect<&linked_stop_source::request_stop>(this);
        }
    } */
};


}}