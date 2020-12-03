#pragma once

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

}}