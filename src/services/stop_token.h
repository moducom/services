#pragma once

// doing this since C++20 is still so new.
// consider putting this into estdlib

namespace moducom { namespace services {

class stop_token
{
    //bool stop_requested_ = false;
    std::atomic<bool> stop_requested_;

    friend class stop_source;

public:
    [[nodiscard]] bool stop_requested() const noexcept
    {
        return stop_requested_;
    }
};


class stop_source
{
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
};


template <class Callback>
class stop_callback
{
public:
    typedef Callback callback_type;
};

}}