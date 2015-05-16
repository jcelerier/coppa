#define CATCH_CONFIG_MAIN
#include<catch.hpp>

#include <coppa/oscquery/json/parser.hpp>
#include <coppa/tools/random.hpp>
using namespace coppa;
using namespace coppa::oscquery;
using namespace coppa::oscquery::JSON;
using namespace eggs::variants;
using namespace std;

TEST_CASE( "path_added parsing", "[parser]" ) {

  GIVEN( "A non-empty parameter map" ) {

    SimpleParameterMap<ParameterMap> map;
    setup_basic_map(map);

    REQUIRE( map.size() == 5 );

    WHEN( "A basic path is added" ) {
      parser::path_added<SimpleParameterMap<ParameterMap>>(map,
      "{ \"path_added\" : { \"full_path\" : \"\\/a\\/b\" \"type\": \"f\" \"access\": 1 } }");

      THEN( "the capacity changes" ) {
        REQUIRE( map.size() == 6 );
      }
      THEN( "the path is correctly added" ) {
        auto p = map.get("/a/b");

        REQUIRE(p.destination == "/a/b" );
        REQUIRE(p.accessmode == Access::Mode::Get );

        REQUIRE(p.values.empty());
        REQUIRE(p.ranges.empty());
        REQUIRE(p.clipmodes.empty());
        REQUIRE(p.description.empty());
        REQUIRE(p.tags.empty());
      }
    }

    WHEN( "A path with an int value is added" ) {
      parser::path_added<SimpleParameterMap<ParameterMap>>(map,
      "{ \"path_added\" : { \"full_path\" : \"\\/a\\/b\" \"type\": \"f\" \"access\": 1 \"value\": [ 15 ], \"range\": [ [0, 100, null] ]} }");

      THEN( "the capacity changes" ) {
        REQUIRE( map.size() == 6 );
      }
      THEN( "the path is correctly added" ) {
        auto p = map.get("/a/b");

        REQUIRE(p.destination == "/a/b" );
        REQUIRE(p.accessmode == Access::Mode::Get );

        REQUIRE(p.values.size() == 1);
        REQUIRE(p.ranges.size() == 1);

        REQUIRE(get<int>(p.values.front()) == 15);
        REQUIRE(get<int>(p.ranges.front().min) == 0);
        REQUIRE(get<int>(p.ranges.front().max) == 100);
        REQUIRE(p.ranges.front().values.empty());

        REQUIRE(p.description.empty());
        REQUIRE(p.tags.empty());
      }
    }

    WHEN( "A path with a float value is added" ) {
      parser::path_added<SimpleParameterMap<ParameterMap>>(map,
      "{ \"path_added\" : { \"full_path\" : \"\\/a\\/b\" \"type\": \"f\" \"access\": 1 \"value\": [ 15.5 ], \"range\": [ [0.3, 10.4, null] ]} }");

      THEN( "the capacity changes" ) {
        REQUIRE( map.size() == 6 );
      }
      THEN( "the path is correctly added" ) {
        auto p = map.get("/a/b");

        REQUIRE(p.destination == "/a/b" );
        REQUIRE(p.accessmode == Access::Mode::Get );

        REQUIRE(p.values.size() == 1);
        REQUIRE(p.ranges.size() == 1);

        REQUIRE(get<float>(p.values.front()) == 15.5f);
        REQUIRE(get<float>(p.ranges.front().min) == 0.3f);
        REQUIRE(get<float>(p.ranges.front().max) == 10.4f);
        REQUIRE(p.ranges.front().values.empty());

        REQUIRE(p.description.empty());
        REQUIRE(p.tags.empty());
      }
    }

    WHEN( "A path with a string and an int value is added" ) {
      parser::path_added<SimpleParameterMap<ParameterMap>>(map,
      "{ \"path_added\" : "
      "    { \"full_path\" : \"\\/a\\/b\" \"type\": \"f\" \"access\": 1 "
      "      \"value\": [ \"yodee\", 45 ],"
      "      \"range\": [ [null, null, [\"yodee\", \"yaho\", ]], [null, null, null] ]} }");

      THEN( "the capacity changes" ) {
        REQUIRE( map.size() == 6 );
      }
      THEN( "the path is correctly added" ) {
        auto p = map.get("/a/b");

        REQUIRE(p.destination == "/a/b" );
        REQUIRE(p.accessmode == Access::Mode::Get );

        REQUIRE(p.values.size() == 2);
        REQUIRE(p.ranges.size() == 2);

        REQUIRE(get<std::string>(p.values.front()) == "yodee");
        const auto& stringRange = p.ranges.front();
        REQUIRE(!stringRange.min);
        REQUIRE(!stringRange.max);
        REQUIRE(stringRange.values.size() == 2);
        REQUIRE(std::find(begin(stringRange.values), end(stringRange.values), Variant(std::string("yodee"))) != end(stringRange.values));
        REQUIRE(std::find(begin(stringRange.values), end(stringRange.values), Variant(std::string("yaho"))) != end(stringRange.values));

        const auto& intRange = p.ranges.back();
        REQUIRE(!intRange.min);
        REQUIRE(!intRange.max);
        REQUIRE(intRange.values.empty());

        REQUIRE(p.description.empty());
        REQUIRE(p.tags.empty());
      }
    }


    WHEN( "An existing path is added again" ) {
      parser::path_added<SimpleParameterMap<ParameterMap>>(map,
      "{ \"path_added\" : "
      "    { \"full_path\" : \"\\/a\\/b\" \"type\": \"f\" \"access\": 1 "
      "      \"value\": [ \"yodee\", 45 ],"
      "      \"range\": [ [null, null, [\"yodee\", \"yaho\", ]], [null, null, null] ]} }");

      THEN( "the capacity changes" ) {
        REQUIRE( map.size() == 6 );
      }
      THEN( "the path is correctly added" ) {
        auto p = map.get("/a/b");

        REQUIRE(p.destination == "/a/b" );
        REQUIRE(p.accessmode == Access::Mode::Get );

        REQUIRE(p.values.size() == 2);
        REQUIRE(p.ranges.size() == 2);

        REQUIRE(get<std::string>(p.values.front()) == "yodee");
        const auto& stringRange = p.ranges.front();
        REQUIRE(!stringRange.min);
        REQUIRE(!stringRange.max);
        REQUIRE(stringRange.values.size() == 2);
        REQUIRE(std::find(begin(stringRange.values), end(stringRange.values), Variant(std::string("yodee"))) != end(stringRange.values));
        REQUIRE(std::find(begin(stringRange.values), end(stringRange.values), Variant(std::string("yaho"))) != end(stringRange.values));

        const auto& intRange = p.ranges.back();
        REQUIRE(!intRange.min);
        REQUIRE(!intRange.max);
        REQUIRE(intRange.values.empty());

        REQUIRE(p.description.empty());
        REQUIRE(p.tags.empty());
      }
    }

  }
}






TEST_CASE( "path_removed parsing", "[parser]" ) {

  GIVEN( "A non-empty parameter map" ) {

    SimpleParameterMap<ParameterMap> map;
    setup_basic_map(map);

    REQUIRE(map.size() == 5 );

    REQUIRE(map.has("/da/da") == true);

    WHEN( "An existing, real leaf path is removed" ) {
      parser::path_removed(map, "{ \"path_removed\" : \"\\/da\\/da\" }");

      THEN( "the capacity changes" ) {
        REQUIRE( map.size() == 4 );
      }
      THEN( "the path isn't there anymore" ) {
        REQUIRE(map.has("/da/da") == false);
      }
    }


    WHEN( "An existing, non-real node path is removed" ) {
      parser::path_removed(map, "{ \"path_removed\" : \"\\/da\" }");

      THEN( "the capacity changes" ) {
        REQUIRE( map.size() == 3 );
      }
      THEN( "the paths aren't there anymore" ) {
        REQUIRE(map.has("/da/da") == false);
        REQUIRE(map.has("/da/do") == false);
      }
    }
  }
}

