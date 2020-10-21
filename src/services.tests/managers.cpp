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

            scheduler.add(&agent1);

            scheduler.run(100);
        }
        SECTION("multiple")
        {
            agents::Periodic<Periodic1> agent1(1000);
            agents::Periodic<Periodic1> agent2(1000);
            agents::Periodic<Periodic1> agent3(1000);

            scheduler.add(&agent1);
            scheduler.add(&agent2);
            scheduler.add(&agent3);

            scheduler.run(100);
        }
    }
}