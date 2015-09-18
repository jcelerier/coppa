#define CATCH_CONFIG_MAIN
#include<catch.hpp>
#include <coppa/oscquery/map.hpp>
#include <coppa/tools/random.hpp>
using namespace coppa;
using namespace coppa::oscquery;
using namespace eggs::variants;
using namespace std;


TEST_CASE( "map construction", "[oscquery][map]" ) {
    basic_map<ParameterMap> map;

    REQUIRE(map.size() == 1);
    REQUIRE(map.has("/") == true);
    REQUIRE(map.get("/").description == "root node");

    // TODO the root node name should be "" in order to remove its special case.
}

TEST_CASE( "map element adding", "[oscquery][map]" ) {

  GIVEN( "A parameter map" ) {

    basic_map<ParameterMap> map;

    WHEN( "A parameter is added as a child of a real node" ) {
      Parameter p = random_anonymous_parameter();
      p.destination = "/da";

      map.insert(p);
      THEN( "the capacity changes" ) {
        REQUIRE( map.size() == 2 );
      }
      THEN( "the parameter is here" ) {
        REQUIRE(map.has("/da") == true);
        REQUIRE(map.existing_path("/da") == true);
      }
    }

    WHEN( "A parameter is added as a child of a fake node" ) {
      Parameter p = random_anonymous_parameter();
      p.destination = "/do/di/da/du";

      map.insert(p);
      THEN( "the capacity changes" ) {
        REQUIRE( map.size() == 2 );
      }
      THEN( "the parameter is here" ) {
        REQUIRE(map.has(p.destination) == true);
        REQUIRE(map.get(p.destination) == p);
      }
      THEN( "the parameter's parent is not here" ) {
        REQUIRE(map.has("/do/di/da") == false);
        REQUIRE(map.existing_path("/do/di/da") == true);
      }
    }

    WHEN( "A parameter is re-added" ) {
      Parameter p1 = random_anonymous_parameter();
      p1.destination = "/do/di/da/du";
      map.insert(p1);

      Parameter p2 = random_anonymous_parameter();
      p2.destination = "/do/di/da/du";
      map.insert(p2);

      THEN( "the capacity does not change" ) {
        REQUIRE( map.size() == 2 );
      }
      THEN( "the parameter is here" ) {
        REQUIRE(map.has(p1.destination) == true);
      }
      THEN("the parameter has not changed") {
        REQUIRE(map.get(p1.destination) == p1);
      }
    }
  }

}



TEST_CASE( "map element updating", "[oscquery][map]" ) {
  GIVEN( "A parameter map" ) {
    basic_map<ParameterMap> map;
    Parameter p1 = random_anonymous_parameter();
    p1.destination = "/do/di/da/du";
    p1.description = "foo";
    p1.tags = {"audio"};

    Parameter p2 = random_anonymous_parameter();
    p2.destination = "/do/di/da/du";
    p2.description = "bar";
    p2.tags = {"video"};

    REQUIRE(p1.destination == p2.destination);
    REQUIRE(p1.description != p2.description);

    map.insert(p1);

    WHEN( "An existing parameter is replaced" )  {
      map.replace(p2);

      THEN( "the capacity does not change" ) {
        REQUIRE( map.size() == 2 );
      }
      THEN( "the parameter is here" ) {
        REQUIRE(map.has(p1.destination) == true);
      }
      THEN("the parameter has changed") {
        REQUIRE(map.get(p1.destination) == p2);
      }
    }

    WHEN( "An existing parameter attribute is updated" )  {
      REQUIRE(map.get(p1.destination) == p1);

      map.update(p1.destination, [&] (Parameter& p) { p.description = p2.description; });

      THEN( "the capacity does not change" ) {
        REQUIRE( map.size() == 2 );
      }
      THEN( "the parameter is here" ) {
        REQUIRE(map.has(p1.destination) == true);
      }
      THEN("only the attribute has changed") {
        REQUIRE(map.get(p1.destination).tags == p1.tags);
        REQUIRE(map.get(p1.destination).description == p2.description);
      }
    }

    WHEN( "An existing parameter destination is updated" )  {
      REQUIRE(map.get(p1.destination) == p1);

      map.update(p1.destination, [] (Parameter& p) { p.destination = "/dee/daa"; });

      THEN( "the capacity does not change" ) {
        REQUIRE( map.size() == 2 );
      }
      THEN( "the destination has changed" ) {
        REQUIRE(map.has(p1.destination) == false);
        REQUIRE(map.has("/dee/daa") == true);
      }
      THEN("the other attributes didn't change") {
        auto param = map.get("/dee/daa");
        param.destination = p1.destination;

        REQUIRE(param == p1);
      }
    }
  }
}


TEST_CASE( "map element removing", "[oscquery][map]" ) {

  GIVEN( "A non-empty parameter map" ) {

    basic_map<ParameterMap> map;
    setup_basic_map(map);

    REQUIRE(map.size() == 5);

    REQUIRE(map.has("/") == true);
    REQUIRE(map.has("/plop") == true);
    REQUIRE(map.has("/plop/plip/plap") == true);
    REQUIRE(map.has("/da/do") == true);
    REQUIRE(map.has("/da/da") == true);

    REQUIRE(map.has("/da") == false); // Not a real node

    WHEN( "An existing, real leaf path is removed" ) {
      map.remove("/da/da");

      THEN( "the capacity changes" ) {
        REQUIRE( map.size() == 4 );
      }
      THEN( "the path isn't there anymore" ) {
        REQUIRE(map.has("/da/da") == false);
      }
    }

    WHEN( "An existing, non-real node path is removed" ) {
      map.remove("/da");

      THEN( "the capacity changes" ) {
        REQUIRE( map.size() == 3 );
      }
      THEN( "the path isn't there anymore" ) {
        REQUIRE(map.has("/da/da") == false);
      }
    }

    WHEN( "A non-existing path is removed" ) {
      map.remove("/iamnotarealnode");

      THEN( "the capacity does not change" ) {
        REQUIRE( map.size() == 5);
      }

    }

    WHEN( "The root node is removed" ) {
      map.remove("/");

      THEN( "the capacity changes" ) {
        REQUIRE( map.size() == 1 );
      }
      THEN( "the path is still there" ) {
        REQUIRE(map.has("/") == true);
      }
    }

    GIVEN("The root node has some data")
    {
      REQUIRE(map.get("/").description == "root node"); // Default text
      map.update("/", [] (Parameter& p) { p.description = "Root Node Data";});
      REQUIRE(map.get("/").description == "Root Node Data");
      WHEN( "The root node is removed" ) {
        map.remove("/");
        THEN( "the data is not there anymore" ) {
          REQUIRE(map.get("/").description == "root node");
        }
      }
    }
  }
}
