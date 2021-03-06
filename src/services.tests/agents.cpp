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

        SECTION("async")
        {
            agents::AsyncEvent<Event1> agent(enttHelper);

            {
                agent.construct(&agent, generator);

                SECTION("brute force")
                {
#if ENABLE_ASYNC
                    auto a = agent.run(1);

                    a.wait();

                    REQUIRE(a.valid());
#else
                    agent.service().run(1);
#endif
                    REQUIRE(agent.service().value_ == 10);
                }SECTION("natural")
                {
                    /*
                     * Doesn't work since 'std::future' won't convert to bool
                    generator.signal.collect([&](int value)
                    {
                    }, 2); */
                    // Doesn't *really* work since std::future destructs and blocks
                    generator.signal.publish(2);
                }

                agent.destruct();
            }
        }
        SECTION("async queue")
        {
            agents::AsyncEventQueue<Event1> agent(enttHelper);

            agent.construct(generator);

            agent.run(1);
            agent.run(2);
            agent.run(3);

            agent.wait();

            REQUIRE(agent.service().value_ == 60);

            agent.destruct();
        }
    }
}