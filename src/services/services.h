#pragma once

#include <moducom/services/status.h>
#include <moducom/semver.h>
#include <moducom/portable_endian.h>

#include <new>
#include <string>
#include <moducom/stop_token.h>

namespace moducom { namespace services {

class Description
{
    const char* name_;
    const SemVer version_;

public:
    Description(const char* name, SemVer version) :
            name_(name), version_(version)
    {}

    Description(Description&& moveFrom) = default;
    Description(const Description& copyFrom) = default;

    Description& operator=(const Description& copyFrom)
    {
        new (this) Description(copyFrom);
        return *this;
    };
};


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


class ServiceToken
{
protected:
    const stop_token& stopToken;

    ServiceToken(const stop_token& stopToken) :
            stopToken(stopToken) {}

public:
    virtual void start() = 0;
};


}}