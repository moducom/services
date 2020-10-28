#include "catch2/catch.hpp"
#include "services.h"

using namespace moducom::services;

struct Listener1
{
    Status status_ = Status::Error;

    void onStatusChanged(agents::BaseBase* agent, Status status)
    {
        status_ = status;
    }
};

static void async1()
{
    std::clog << "hi2u from async1" << std::endl;
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
    }
    SECTION("BaseBase")
    {
        agents::BaseBase base(enttHelper);
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
}