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
    Waiting,    ///< Running, but waiting on a service-specific signal
    Pausing,
    Paused,
    Stopping,
    Stopped,

    Error
};

// EXPERIMENTAL
enum class RunningStatus
{
    Nominal,
    Degraded,
    Waiting
};

inline bool is_running(Status status)
{
    return status == Status::Running || status == Status::Degraded || status == Status::Waiting;
}

}}