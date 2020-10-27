#include "catch2/catch.hpp"
#include "services.h"

using namespace moducom::services;


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

        REQUIRE(generator.sink.empty());
    }
}