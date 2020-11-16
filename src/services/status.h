#pragma once

namespace moducom { namespace services {

enum class Status
{
    Unstarted,
    WaitingOnDependency,
    Starting,
    Started,
    Running,
    Degraded,   ///< Partially running state
    Pausing,
    Paused,
    Stopping,
    Stopped,

    Error
};

inline bool is_running(Status status)
{
    return status == Status::Running || status == Status::Degraded;
}

}}