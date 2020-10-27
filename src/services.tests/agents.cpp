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

        //try
        {
            // FIX: Crashing, don't know why
            
            agent.construct(generator);

            auto a = agent.onEvent(1);

            //a.get();
            a.wait();

            REQUIRE(agent.service().value_ == 10);

            agent.destruct();
        }
        //catch(const std::exception& e)
        {

        }
    }
}