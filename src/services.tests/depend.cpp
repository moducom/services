#include "catch2/catch.hpp"
#include "services.h"

using namespace moducom::services;

TEST_CASE("dependency")
{
    EventGenerator eventGenerator;
    entt::registry registry;
    agents::EnttHelper enttHelper(registry, registry.create());

    agents::Event<Event1> agent1(enttHelper);
    agents::Event<Event1> agent2(enttHelper);

    SECTION("Depends 1")
    {
        agents::Depender depender;

        depender.add(agent1);

        REQUIRE(depender.anyNotRunning());
        REQUIRE(!depender.satisfied());

        depender.add(agent2);

        agent1.construct(eventGenerator);

        REQUIRE(depender.anyNotRunning());
        REQUIRE(!depender.satisfied());

        agent2.construct(eventGenerator);

        REQUIRE(!depender.anyNotRunning());
        REQUIRE(depender.satisfied());

        agent1.destruct();
        agent2.destruct();
    }
}