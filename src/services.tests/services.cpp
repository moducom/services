#include "catch2/catch.hpp"
#include <service.hpp>

using namespace moducom::services;


struct EventGenerator
{
    entt::sigh<void (int)> signal;
    entt::sink<void (int)> sink;

    EventGenerator() :
            sink{signal}
    {}
};

struct Event1 : ServiceBase
{
    entt::scoped_connection connection;

    int value_ = 0;

    Event1(EventGenerator& generator)
    {
        connection = generator.sink.connect<&Event1::run>(this);
    }

    void run(int value)
    {
        value_ = value * 10;
    }
};



TEST_CASE("raw services")
{
    SECTION("event")
    {
        EventGenerator generator;

        SECTION("raw service")
        {
            Event1 service(generator);

            REQUIRE(service.value_ == 0);

            generator.signal.publish(2);

            REQUIRE(service.value_ == 20);
        }
        SECTION("service through agent")
        {

        }
    }
}