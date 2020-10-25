#include "catch2/catch.hpp"
#include <service.hpp>

using namespace moducom::services;

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

        entt::sink sink{base.statusSignal()};
    }
}