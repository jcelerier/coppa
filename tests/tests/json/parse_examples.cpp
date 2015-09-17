#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <coppa/oscquery/device/remote.hpp>
#include <coppa/oscquery/json/parser.hpp>
#include <coppa/tools/random.hpp>
using namespace coppa;
using namespace coppa::oscquery;
using namespace coppa::oscquery::JSON;
using namespace eggs::variants;
using namespace std;
#include <dirent.h>

std::vector<std::string> list_test_files()
{
    std::vector<std::string> vec;

    if (auto dir = opendir("json_files"))
    {
      while (auto ent = readdir(dir))
      {
          vec.push_back(std::string(ent->d_name));
      }
      closedir(dir);
    }

    return vec;
}

std::string read_json_file(const std::string& filename)
{
    std::ifstream t(filename);
    std::stringstream buffer;
    buffer << t.rdbuf();

    return buffer.str();
}

class query_protocol_fake
{
        std::function<void(const std::string&)> m_callback;
    public:
        query_protocol_fake(
                const std::string&,
                std::function<void(const std::string&)> cb):
            m_callback{cb}
        {

        }

        void message(const std::string& mess)
        {
            m_callback(mess);
        }

        std::string uri() const
        {
            return {};
        }
};

class test_remote_device : public QueryRemoteDevice<
    SimpleParameterMap<ParameterMap>,
    JSON::parser,
    query_protocol_fake,
    SettableMap<SimpleParameterMap<ParameterMap>, osc::sender>>
{
  public:
    using QueryRemoteDevice::QueryRemoteDevice;
    void set(const std::string& addr, oscquery::Values& val)
    {
      SettableMap::set(addr, val.values);
    }
};

TEST_CASE( "parse all files without crashing", "[parser]" ) {

    for(const auto& file : list_test_files())
    {
        test_remote_device dev("");
        dev.message(read_json_file("json_files" + file));
    }
}


TEST_CASE( "various parse examples", "[parser]" ) {

    for(const auto& file : list_test_files())
    {
        test_remote_device dev("");
        dev.message(read_json_file("json_files" + file));
    }
}


template<typename Map>
bool operator==(const Map& lhs, const Map& rhs)
{
    bool b = lhs.size() == rhs.size();

    if(!b)
        return false;

    for(auto& elt : lhs)
    {
        if(rhs.has(elt.destination))
        {
            if(elt != rhs.get(elt.destination))
                return false;
        }
        else
        {
            return false;
        }
    }

    return true;
}

TEST_CASE( "path_removed parsing", "[parser]" ) {

  GIVEN( "An empty parameter map" ) {

      test_remote_device dev("");
      auto map = dev.safeMap().unsafeMap(); // Source copy


      WHEN( "empty.json read" ) {
        dev.message(read_json_file("json_files/empty.json"));

        THEN("nothing happens")
        {
            bool map_eq = map == dev.safeMap().unsafeMap();
            std::cerr << map.size() << " " << dev.safeMap().unsafeMap().size();
            REQUIRE( map_eq );
        }
      }
      WHEN( "null.json read" ) {
          dev.message(read_json_file("json_files/null.json"));

          THEN("nothing happens")
          {
              bool map_eq = map == dev.safeMap().unsafeMap();
              std::cerr << map.size() << " " << dev.safeMap().unsafeMap().size();
              REQUIRE( map_eq );
          }
      }
      WHEN( "root.json read" ) {
          dev.message(read_json_file("json_files/root.json"));

          THEN("nothing happens")
          {
              bool map_eq = map == dev.safeMap().unsafeMap();
              std::cerr << map.get("/").description << " " << dev.safeMap().unsafeMap().size();
              REQUIRE( map_eq );
          }
      }
      WHEN( "web_example1.json read" ) {
          dev.message(read_json_file("json_files/web_example1.json"));

          THEN("the map changes")
          {
              bool map_eq = map == dev.safeMap().unsafeMap();
              std::cerr << dev.get("/baz").description << std::endl;
              REQUIRE( !map_eq );
          }
      }
  }
}
