#include "catch2/catch.hpp"
#include <service.hpp>

using namespace moducom::services;

struct Periodic1
{
    void run(int interval)
    {

    }
};

TEST_CASE("managers")
{
    SECTION("scheduler")
    {
        managers::Scheduler<int> scheduler;

        agents::Periodic<Periodic1> agent1(1000);

        scheduler.add(&agent1);

        scheduler.run(100);
    }
}