#include "catch2/catch.hpp"
#include "services.h"

using namespace moducom::services;

TEST_CASE("dependency")
{
    entt::registry registry;
    agents::EnttHelper enttHelper(registry, registry.create());

    agents::Event<Event1> agent1(enttHelper);
    agents::Event<Event1> agent2(enttHelper);

    SECTION("Depends 1")
    {
        agents::Depender depender;

        depender.add(agent1);
    }
}