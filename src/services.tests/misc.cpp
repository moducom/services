#include "catch2/catch.hpp"
#include "services.h"

using namespace moducom::services;

struct Listener1
{
    Status status_ = Status::Error;

    void onStatusChanged(agents::Agent* agent, Status status)
    {
        status_ = status;
    }
};

static void async1()
{
    std::clog << "hi2u from async1" << std::endl;
}

static void requireSeven(int value)
{
    REQUIRE(value == 7);
}

static void methodWithReference(int&) {}

void async2()
{
    std::clog << "hi2u from async2" << std::endl;
}


TEST_CASE("misc")
{
    entt::registry registry;
    agents::EnttHelper enttHelper(registry, registry.create());

    SECTION("A")
    {

    }
    SECTION("stop_source")
    {
        stop_source s;

        SECTION("stop_callback")
        {
            bool called_back = false;

            stop_callback c(s.token(), [&]
            {
                called_back = true;
            });

            s.request_stop();

            REQUIRE(called_back);
        }
    }
    SECTION("Agent")
    {
        agents::Agent base(enttHelper);
        Listener1 listener1;

        base.statusSink.connect<&Listener1::onStatusChanged>(listener1);

        base.status(Status::Running);

        REQUIRE(base.status() == listener1.status_);
    }
    SECTION("std::async")
    {
        SECTION("standalone")
        {
            // because we're having such difficulty elsewhere
            auto a = std::async(std::launch::async, async1);

            a.wait();
        }
        SECTION("instance method")
        {
            EventGenerator generator;
            Event1 event1(generator);

            auto a = std::async(std::launch::async, &Event1::run, &event1, 1);

            a.wait();

            REQUIRE(event1.value_ == 10);
        }
    }
    SECTION("internal::BlockingQueue")
    {
        using namespace std::chrono_literals;

        agents::internal::BlockingQueue<int> bq;

        SECTION("just wait")
        {
            bool item_present = bq.wait_for_presence(50ms);
            REQUIRE(!item_present);
        }
        SECTION("no waiting")
        {
            bq.push(1);
            bool item_present = bq.wait_for_presence(50ms);
            REQUIRE(item_present);
        }
        SECTION("some waiting")
        {
            bq.push(1);
            bool item_present = bq.wait_for_presence(50ms);
            REQUIRE(item_present);
            {
                auto lock = bq.unique_lock();
                REQUIRE(bq.q().front() == 1);
                bq.q().pop();
                REQUIRE(bq.q().empty());
            }
            item_present = bq.wait_for_presence(50ms);
            REQUIRE(!item_present);
        }
        SECTION("emplace")
        {
            bq.emplace(1);
        }
    }
    SECTION("internal")
    {
        SECTION("ArgType")
        {
            SECTION("ArgType 1")
            {
                typedef moducom::internal::ArgType<decltype(&Event1::run)> argtype;
                typedef argtype::tuple_type test_type;

                test_type tuple;

                std::get<0>(tuple) = 7;

                argtype::invoke(requireSeven, tuple);
            }
            SECTION("ArgType 2")
            {
                moducom::internal::ArgType<decltype(&Listener1::onStatusChanged)>::tuple_type tuple;

                auto& item1 = std::get<0>(tuple);
                auto& item2 = std::get<1>(tuple);

                REQUIRE(item2 == Status::Unstarted);
            }
            SECTION("reference tests")
            {
                typedef moducom::internal::ArgType<decltype(&methodWithReference)>::tuple_type tuple_type;

                REQUIRE(std::is_reference_v<std::tuple_element<0, tuple_type>::type>);
            }
            SECTION("reference_removed tests")
            {
                typedef moducom::internal::ArgType<decltype(&methodWithReference)>::tuple_remove_reference_type tuple_type;

                REQUIRE(!std::is_reference_v<std::tuple_element<0, tuple_type>::type>);
            }
        }
    }
}