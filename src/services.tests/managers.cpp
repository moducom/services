#include "catch2/catch.hpp"

#include <services/service.hpp>

#include <chrono>

#include <cstring>

using namespace moducom::services;

typedef moducom::services::agents::Agent agent_type;

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

    bool operator ==(const basic_int_duration& compareTo) const
    {
        return value == compareTo.value;
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


struct Scheduled1 : ServiceBase
{
    typedef fake_clock::duration duration_type;

    duration_type run(duration_type interval)
    {
        return duration_type(1000);
    }
};

struct Continuous1 : ServiceBase
{
    const int value;

    Continuous1(int value) : value(value) {}

    void run()
    {
        using namespace std::chrono_literals;

        // DEBT: Need our own stop_source interruptable sleep
        std::this_thread::sleep_for(100ms);
    }
};


struct ScheduledRemoval : ServiceBase
{
    typedef fake_clock::duration duration_type;
    int counter = 3;

    duration_type run(duration_type interval)
    {
        if(--counter)
            return duration_type(1000);
        else
            return duration_type::min();
    }
};


TEST_CASE("managers")
{
    entt::registry registry;
    agents::EnttHelper enttHelper(registry, registry.create());
    agents::EnttHelper entity2(registry, registry.create());

    SECTION("semver")
    {
        //enttHelper.registry.get<moducom::SemVer>(enttHelper.entity);
    }
    SECTION("aggregator")
    {
        agents::Aggregator a(enttHelper);
    }
    SECTION("scheduler")
    {
        /*
        SECTION("basic")
        {
            managers::Scheduler<int, int> scheduler;

            agents::Periodic<Periodic1<int> > agent1(1000);

            scheduler.add(&agent1, 0);

            scheduler.run(100);
        } */
        SECTION("multiple")
        {
            managers::Scheduler<fake_clock::time_point, fake_clock::duration> scheduler;

            agents::Periodic<Periodic1<fake_clock::duration> > agent1(enttHelper, 1000);
            agents::Periodic<Periodic1<fake_clock::duration> > agent2(enttHelper, 1000);
            agents::Periodic<Periodic1<fake_clock::duration> > agent3(enttHelper, 1000);

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
        SECTION("real chrono")
        {
            using namespace std::chrono_literals;

            typedef std::chrono::system_clock clock_type;
            managers::Scheduler<clock_type::time_point, clock_type::duration> scheduler;

            agents::Periodic<Periodic1<clock_type::duration> > agent1(enttHelper, 1000ms);

            scheduler.add(&agent1, clock_type::time_point(200ms));

            scheduler.run(clock_type::time_point(100ms));
            REQUIRE(scheduler.cfirst().wakeup == clock_type::time_point(200ms));
            scheduler.run(clock_type::time_point(300ms));
            REQUIRE(scheduler.cfirst().wakeup == clock_type::time_point(1200ms));
        }
        SECTION("scheduled")
        {
            managers::Scheduler<fake_clock::time_point, fake_clock::duration> scheduler;

            SECTION("basic")
            {
                agents::ScheduledRelative<Scheduled1> agent1(enttHelper);

                scheduler.add(&agent1, 500);
            }
            SECTION("remover")
            {
                agents::ScheduledRelative<ScheduledRemoval> agent1(enttHelper);

                agent1.construct();

                scheduler.add(&agent1, 500);

                scheduler.run(1000);
                scheduler.run(2000);
                REQUIRE(scheduler.count() == 1);
                scheduler.run(3000);

                REQUIRE(scheduler.count() == 0);

                agent1.destruct();
            }
        }
    }
    SECTION("standalone")
    {
        auto s = agents::make_standalone<Continuous1>(enttHelper, 1);

        memset(&s.service(), 0, sizeof(Continuous1));

        REQUIRE(s.service().value == 0);

        s.construct();

        REQUIRE(s.service().value == 1);

        SECTION("threading")
        {
            using namespace std::chrono_literals;

            stop_source source;

            // remember, = && moves ownership of a thread
            std::thread worker = s.run(source.token());

            Status status = s.status();

            REQUIRE(
                (status == Status::Started ||
                status == Status::Starting ||
                status == Status::Running));

            // DEBT: Spinwaits are bad
            while(s.status() != Status::Running)
            {
                std::this_thread::sleep_for(50ms);
            }

            source.request_stop();

            worker.join();

            REQUIRE(s.status() == Status::Stopped);
        }
        SECTION("manager")
        {
            managers::StandaloneStdThreadManager manager(enttHelper);

            auto serviceToken = manager.push<Continuous1>(2);

            serviceToken.start();

            // FIX: If we don't stop(), some thread-related errors occur here mainly complaining
            // from entt, likely because entt registry itself goes out of scope before thread
            // does -- though I really don't recall interacting with registry on shutdown that much
            manager.stop();
        }
    }
}