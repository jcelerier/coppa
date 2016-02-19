#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <coppa/minuit/parameter.hpp>
#include <coppa/tools/random.hpp>
using namespace coppa;
using namespace coppa::ossia;
using namespace eggs::variants;
using namespace std;




TEST_CASE( "value init", "[ossia][value]" ) {
    coppa::ossia::Variant v;
    Values vals_in;
    vals_in.variants = {1, 2, 3};
    v = vals_in;
    auto vals_out = get<Values>(v);

    REQUIRE(vals_out == vals_in);
}

TEST_CASE( "parameter init", "[ossia][value]" ) {
    coppa::ossia::Parameter p;
}
