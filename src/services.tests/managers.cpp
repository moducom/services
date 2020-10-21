#include "catch2/catch.hpp"
#include <service.hpp>

using namespace moducom::services;

struct Periodic1
{
    typedef int duration_type;

    void run(duration_type interval)
    {

    }
};

TEST_CASE("managers")
{
    SECTION("scheduler")
    {
        managers::Scheduler<int, int> scheduler;

        SECTION("basic")
        {
            agents::Periodic<Periodic1> agent1(1000);

            scheduler.add(&agent1, 0);

            scheduler.run(100);
        }
        SECTION("multiple")
        {
            agents::Periodic<Periodic1> agent1(1000);
            agents::Periodic<Periodic1> agent2(1000);
            agents::Periodic<Periodic1> agent3(1000);

            scheduler.add(&agent1,0);
            scheduler.add(&agent2, 500);
            scheduler.add(&agent3, 750);

            REQUIRE(scheduler.cfirst().wakeup == 0);
            scheduler.run(100);
            REQUIRE(scheduler.cfirst().wakeup == 500);
            scheduler.run(200);
            REQUIRE(scheduler.cfirst().wakeup == 500);
            scheduler.run(501);
            REQUIRE(scheduler.cfirst().wakeup == 750);
            scheduler.run(700);
            scheduler.run(750);
            REQUIRE(scheduler.cfirst().wakeup == 1000);
        }
    }
}