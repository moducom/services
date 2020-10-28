#include "catch2/catch.hpp"
#include "services.h"

using namespace moducom::services;

#define ENABLE_ASYNC 1

TEST_CASE("agents")
{
    entt::registry registry;
    agents::EnttHelper enttHelper(registry, registry.create());
    SECTION("sanity checks")
    {
        EventGenerator generator;

        agents::AsyncEvent<Event1> agent(enttHelper);

        void* service = &agent.service();
        agents::AsyncEvent<Event1>* _agent = &agent;
    }
    SECTION("event")
    {
        using namespace std::chrono_literals;

        EventGenerator generator;

        agents::AsyncEvent<Event1> agent(enttHelper);

        {
            agent.construct(generator);

            {
#if ENABLE_ASYNC
                auto a = agent.onEvent(1);

                a.wait();

                REQUIRE(a.valid());
#else
                agent.service().run(1);
#endif
                REQUIRE(agent.service().value_ == 10);
            }

            agent.destruct();
        }
    }
}