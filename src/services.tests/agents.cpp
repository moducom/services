#include "catch2/catch.hpp"
#include "services.h"

using namespace moducom::services;

TEST_CASE("agents")
{
    entt::registry registry;
    agents::EnttHelper enttHelper(registry, registry.create());
    SECTION("event")
    {
        EventGenerator generator;

        agents::AsyncEvent<Event1> agent(enttHelper);

        //auto a = agent.onEvent(1);
        //a.wait();
    }
}