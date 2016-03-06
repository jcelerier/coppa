#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <coppa/ossia/parameter.hpp>
#include <coppa/ossia/device/message_handler.hpp>
#include <coppa/ossia/device/osc_local_device.hpp>
#include <coppa/tools/random.hpp>
using namespace coppa;
using namespace coppa::ossia;
using namespace eggs::variants;
using namespace std;




TEST_CASE( "value init", "[ossia][value]" ) {
    coppa::ossia::Variant v;
    Tuple vals_in;
    vals_in.variants = {1, 2, 3};
    v = vals_in;
    auto vals_out = get<Tuple>(v);

    REQUIRE(vals_out == vals_in);
}
