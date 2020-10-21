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

    bool operator ==(TInt compareTo) const
    {
        return value == compareTo;
    }

    basic_int_duration& operator +=(const basic_int_duration& summand)
    {
        value += summand.value;
        return *this;
    }

    basic_int_duration(TInt value) :
        value(value)
    {}
};

template <typename TInt>
basic_int_duration<TInt> operator -(
        const basic_int_duration<TInt>& main,
        const basic_int_duration<TInt>& subtrahend)
{
    return basic_int_duration<TInt>(main.value - subtrahend.value);
}



struct fake_clock
{
    typedef basic_int_duration<int> duration;
    typedef duration time_point;
};

template <typename TDuration>
struct Periodic1 : ServiceBase
{
    typedef TDuration duration_type;

    void run(duration_type interval)
    {

    }
};

TEST_CASE("managers")
{
    SECTION("scheduler")
    {
        SECTION("basic")
        {
            managers::Scheduler<int, int> scheduler;

            agents::Periodic<Periodic1<int> > agent1(1000);

            scheduler.add(&agent1, 0);

            scheduler.run(100);
        }
        SECTION("multiple")
        {
            managers::Scheduler<fake_clock::time_point, fake_clock::duration> scheduler;

            agents::Periodic<Periodic1<fake_clock::duration> > agent1(1000);
            agents::Periodic<Periodic1<fake_clock::duration> > agent2(1000);
            agents::Periodic<Periodic1<fake_clock::duration> > agent3(1000);

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