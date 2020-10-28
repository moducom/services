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

        //try
        {
            agent.construct(generator);

            {
                Event1* service = &agent.service();
                agents::AsyncEvent<Event1>* _agent = &agent;

#if ENABLE_ASYNC
                auto a = agent.onEvent(1);

                //a.get();
                a.wait();

                REQUIRE(a.valid());
#else
                agent.service().run(1);
#endif
                REQUIRE(agent.service().value_ == 10);
            }

            // doesn't help
            //std::this_thread::sleep_for(500ms);

            // FIX: Crashing, don't know why
            agent.destruct();
        }
        //catch(const std::exception& e)
        {

        }
    }
}