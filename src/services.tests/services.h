#pragma once

#include <services/service.hpp>
#include <iostream>

struct EventGenerator
{
    entt::sigh<void (int)> signal;
    entt::sink<void (int)> sink;

    EventGenerator() :
            sink{signal}
    {}
};

struct Event1 : moducom::services::ServiceBase
{
    //entt::scoped_connection connection;

    int value_ = 0;

    EventGenerator& generator;

    Event1(EventGenerator& generator) : generator(generator)
    {
        //connection =
                generator.sink.connect<&Event1::run>(this);
    }

    ~Event1()
    {
        generator.sink.disconnect<&Event1::run>(this);
        //std::clog << "Got here" << std::endl;
    }

    void run(int value)
    {
        value_ = value * 10;
    }
};

