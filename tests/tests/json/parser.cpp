#define CATCH_CONFIG_MAIN
#include <catch.hpp>

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
      parser::path_added<SimpleParameterMap<ParameterMap>>(
            map,
   R"_json_(
      { "PATH_ADDED" : { "FULL_PATH" : "/a/b" "TYPE": "i" "ACCESS": 1 } }
   )_json_");

      THEN( "the capacity changes" ) {
        REQUIRE( map.size() == 6 );
      }
      THEN( "the path is correctly added" ) {
        auto p = map.get("/a/b");

        REQUIRE(p.destination == "/a/b" );
        REQUIRE(p.accessmode == Access::Mode::Get );

        REQUIRE(p.values.size() == 1);
        REQUIRE(p.values.front().which() == Variant(5).which());
        REQUIRE(p.ranges.size() == 1);
        REQUIRE(p.clipmodes.size() == 1);
        REQUIRE(p.description.empty());
        REQUIRE(p.tags.empty());
      }
    }

    WHEN( "A path with an int value is added" ) {
      parser::path_added<SimpleParameterMap<ParameterMap>>(
            map,
    R"_json_(
      { "PATH_ADDED" : { "FULL_PATH" : "/a/b" "TYPE": "i" "ACCESS": 1 "VALUE": [ 15 ], "RANGE": [ [0, 100, null] ]} }
    )_json_");

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
      parser::path_added<SimpleParameterMap<ParameterMap>>(
            map,
      R"_json_(
        { "PATH_ADDED" : { "FULL_PATH" : "/a/b" "TYPE": "f" "ACCESS": 1 "VALUE": [ 15.5 ], "RANGE": [ [0.3, 10.4, null] ]} }
      )_json_");

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
      parser::path_added<SimpleParameterMap<ParameterMap>>(
            map,
       R"_json_(
       { "PATH_ADDED" :
         { "FULL_PATH" : "\/a\/b" "TYPE": "si" "ACCESS": 1
           "VALUE": [ "yodee", 45 ],
           "RANGE": [ [null, null, ["yodee", "yaho", ]], [null, null, null] ]
         }
       }
       )_json_");

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
      parser::path_added<SimpleParameterMap<ParameterMap>>(
            map,
         R"_json_(
         { "PATH_ADDED" :
          { "FULL_PATH" : "\/a\/b" "TYPE": "si" "ACCESS": 1
            "VALUE": [ "yodee", 45 ],
            "RANGE": [ [null, null, ["yodee", "yaho", ]], [null, null, null] ]
          }
         }
         )_json_");

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

// TODO test types not matching the actual values
// TODO test without "type" attribute

TEST_CASE( "paths_added parsing", "[parser]" ) {

  GIVEN( "A non-empty parameter map" ) {

    SimpleParameterMap<ParameterMap> map;
    setup_basic_map(map);

    REQUIRE( map.size() == 5 );

    WHEN( "Paths are added" ) {
      parser::paths_added<SimpleParameterMap<ParameterMap>>(
            map,
                  R"_json_(
                  { "PATHS_ADDED" : [
                      { "FULL_PATH" : "/a/b" "TYPE": "f" "ACCESS": 1 },
                      { "FULL_PATH" : "/c/d" "TYPE": "si" "ACCESS": 2
                        "VALUE": [ "yodee", 45 ],
                        "RANGE": [ [null, null, ["yodee", "yaho", ]], [null, null, null] ] } ]
                  }
                  )_json_"
);

      THEN( "the capacity changes" ) {
        REQUIRE( map.size() == 7 );
      }
      THEN( "the paths are correctly added" ) {
        auto p = map.get("/a/b");

        REQUIRE(p.destination == "/a/b" );
        REQUIRE(p.accessmode == Access::Mode::Get );

        REQUIRE(p.values.size() == 1);
        REQUIRE(p.ranges.size() == 1);
        REQUIRE(p.clipmodes.size() == 1);
        REQUIRE(p.description.empty());
        REQUIRE(p.tags.empty());


        auto p2 = map.get("/c/d");

        REQUIRE(p2.destination == "/c/d" );
        REQUIRE(p2.accessmode == Access::Mode::Set );

        REQUIRE(p2.values.size() == 2);
        REQUIRE(p2.ranges.size() == 2);

        REQUIRE(get<std::string>(p2.values.front()) == "yodee");
        const auto& stringRange = p2.ranges.front();
        REQUIRE(!stringRange.min);
        REQUIRE(!stringRange.max);
        REQUIRE(stringRange.values.size() == 2);
        REQUIRE(std::find(begin(stringRange.values), end(stringRange.values), Variant(std::string("yodee"))) != end(stringRange.values));
        REQUIRE(std::find(begin(stringRange.values), end(stringRange.values), Variant(std::string("yaho"))) != end(stringRange.values));

        const auto& intRange = p2.ranges.back();
        REQUIRE(!intRange.min);
        REQUIRE(!intRange.max);
        REQUIRE(intRange.values.empty());

        REQUIRE(p2.description.empty());
        REQUIRE(p2.tags.empty());
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
      parser::path_removed(map,
                         R"_json_(
                           { "PATH_REMOVED" : "/da/da" }
                         )_json_");

      THEN( "the capacity changes" ) {
        REQUIRE( map.size() == 4 );
      }
      THEN( "the path isn't there anymore" ) {
        REQUIRE(map.has("/da/da") == false);
      }
    }


    WHEN( "An existing, non-real node path is removed" ) {
      parser::path_removed(map,
                       R"_json_(
                         { "PATH_REMOVED" : "/da" }
                       )_json_");

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

TEST_CASE( "paths_removed parsing", "[parser]" ) {

  GIVEN( "A non-empty parameter map" ) {

    SimpleParameterMap<ParameterMap> map;
    setup_basic_map(map);

    REQUIRE(map.size() == 5 );

    REQUIRE(map.has("/da/da") == true);

    WHEN( "An existing, real leaf path is removed" ) {
      parser::paths_removed(map,
                    R"_json_(
                      { "PATHS_REMOVED" : ["/da/da", "/da/do"] }
                    )_json_");

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


TEST_CASE( "path_changed parsing", "[parser]" ) {

  GIVEN( "A non-empty parameter map" ) {

    SimpleParameterMap<ParameterMap> map;
    setup_basic_map(map);

    REQUIRE(map.size() == 5 );

    REQUIRE(map.has("/da/da") == true);
    REQUIRE(map.get("/da/da").values.front().which() == 0); // It's an int

    WHEN( "A leaf is changed" ) {
      parser::path_changed(
            map,
        R"_json_(
            { "PATH_CHANGED" : { "FULL_PATH" : "/da/da" "TYPE": "f" "ACCESS": 2 } }
        )_json_");

      THEN( "the capacity does not change" ) {
        REQUIRE( map.size() == 5 );
      }
      THEN( "the path has changed" ) {
        REQUIRE(map.has("/da/da") == true);
        REQUIRE(map.get("/da/da").values.front().which() == 1); // It's a float
      }
    }

    WHEN( "Nodes are removed" ) {
      parser::path_changed(
            map,
       R"_json_(
           { "PATH_CHANGED" : { "FULL_PATH" : "/da" } }
       )_json_");

      THEN( "the capacity does change" ) {
        REQUIRE( map.size() == 4 ); // -2 +1
      }
      THEN( "the map has changed" ) {
        REQUIRE(map.has("/da") == true);
        REQUIRE(map.has("/da/da") == false);
        REQUIRE(map.has("/da/do") == false);
      }
    }
  }
}

