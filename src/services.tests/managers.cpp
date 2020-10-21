#include "catch2/catch.hpp"
#include <service.hpp>

using namespace moducom::services;

// for scenarios where std::chrono::duration is a little more than we want
template <class TInt>
struct basic_int_duration
{
    TInt value;

    static constexpr basic_int_duration zero()
    {
        return basic_int_duration {0};
    }

    static constexpr basic_int_duration min()
    {
        return basic_int_duration {-1};
    }

    bool operator >=(const basic_int_duration& compareTo) const
    {
        return value >= compareTo.value;
    }

    basic_int_duration& operator +=(const basic_int_duration& summand)
    {
        value += summand.value;
        return *this;
    }
};

struct fake_clock
{
    typedef basic_int_duration<int> duration;
};

struct Periodic1 : ServiceBase
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