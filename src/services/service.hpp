#pragma once

#include "semver.h"
#include <utility>

namespace moducom { namespace services {

class Manager
{

};


class Description
{
    const char* name_;
    const SemVer version_;

public:
    Description(char* name, SemVer version) :
        name_(name), version_(version)
    {}
};

namespace agents {

template <class TService>
class Base
{
    std::aligned_storage<sizeof(TService), alignof(TService)> raw;

protected:
    typedef TService service_type;
    typedef service_type& reference;

    reference service()
    {
        return reinterpret_cast<reference>(raw);
    }
};

template <class TService>
class SingleShot : public Base<TService>
{

public:
};


template <class TService>
class Periodic : public Base<TService>
{

};

}

}}