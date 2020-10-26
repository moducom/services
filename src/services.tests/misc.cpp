#include "catch2/catch.hpp"
#include <service.hpp>

using namespace moducom::services;

struct Listener1
{
    Status status_ = Status::Error;

    void onStatusChanged(Status status)
    {
        status_ = status;
    }
};

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
}