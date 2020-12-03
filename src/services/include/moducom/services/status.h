#pragma once

#include <string>

namespace moducom { namespace services {

enum class Status
{
    Unstarted,
    WaitingOnDependency,
    Starting,
    Started,
    Running,
    Degraded,   ///< Partially running state
    Waiting,    ///< Running, but waiting on a services-specific signal
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


struct Progress
{
    const short percent;
    const char* subsystem;
    const std::string message;
};


struct Alert
{
    enum Level
    {
        Critical,
        Error,
        Warning,
    };

    const std::string message;
    const char* subsystem;
    const Level level;
};



}}