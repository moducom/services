#pragma once

namespace moducom { namespace services {

enum class Status
{
    Unstarted,
    WaitingOnDependency,
    Starting,
    Started,
    Running,
    Pausing,
    Paused,
    Stopping,
    Stopped,

    Error
};

}}