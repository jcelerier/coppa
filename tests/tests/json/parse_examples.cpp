#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <coppa/oscquery/device/remote.hpp>
#include <coppa/oscquery/json/parser.hpp>
#include <coppa/tools/random.hpp>
using namespace coppa;
using namespace coppa::oscquery;
using namespace coppa::oscquery::json;
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
          auto str = std::string(ent->d_name);
          if(str != "." && str != "..")
            vec.push_back(str);
      }
      closedir(dir);
    }

    return vec;
}

std::vector<std::string> conformance_test_folders()
{
  std::vector<std::string> vec;

  if (auto dir = opendir("conformance"))
  {
    while (auto ent = readdir(dir))
    {
      auto str = std::string(ent->d_name);
      if(str != "." && str != "..")
        vec.push_back(str);
    }
    closedir(dir);
  }

  return vec;
}

auto read_json_map(const std::string& filename)
{
  return json_map{json_file{filename}};
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

class test_remote_device : public remote_query_device<
    basic_map<ParameterMap>,
    json::parser,
    query_protocol_fake,
    remote_map_setter<locked_map<basic_map<ParameterMap>>, osc::sender>>
{
  public:
    using remote_query_device::remote_query_device;
    void set(const std::string& addr, oscquery::Values& val)
    {
      m_setter.set(addr, val.values);
    }
};

TEST_CASE( "parse all files without crashing", "[parser]" ) {

  using namespace coppa;
  for(const auto& file : list_test_files())
  {
    basic_map<ParameterMap> map;
    locked_map<basic_map<ParameterMap>> lm(map);
    remote_map_setter<locked_map<basic_map<ParameterMap>>, osc::sender>
        setter(lm);
    test_remote_device dev(setter, "");
    dev.message(read_json_file("json_files" + file));
  }
}


template<typename Map, typename std::enable_if_t<std::is_class<typename Map::value_type>::value>* = nullptr>
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

TEST_CASE( "other examples parsing", "[parser]" ) {

  GIVEN( "An empty parameter map" ) {
    
      basic_map<ParameterMap> orig_map;
      locked_map<basic_map<ParameterMap>> lm(orig_map);
      remote_map_setter<locked_map<basic_map<ParameterMap>>, osc::sender>
        setter(lm);
      test_remote_device dev(setter, "");
      auto map = dev.map().get_data_map();


      WHEN( "empty.json read" ) {
        dev.message(read_json_file("json_files/empty.json"));

        THEN("nothing happens")
        {
            bool map_eq = map == dev.map().get_data_map();
            std::cerr << map.size() << " " << dev.map().size();
            REQUIRE( map_eq );
        }
      }
      WHEN( "null.json read" ) {
          dev.message(read_json_file("json_files/null.json"));

          THEN("nothing happens")
          {
              bool map_eq = map == dev.map().get_data_map();
              std::cerr << map.size() << " " << dev.map().size();
              REQUIRE( map_eq );
          }
      }
      WHEN( "root.json read" ) {
          dev.message(read_json_file("json_files/root.json"));

          THEN("nothing happens")
          {
              bool map_eq = map == dev.map().get_data_map();
              std::cerr << map.get("/").description << " " << dev.map().size();
              REQUIRE( map_eq );
          }
      }
      WHEN( "web_example1.json read" ) {
          dev.message(read_json_file("json_files/web_example1.json"));

          THEN("the map changes")
          {
              bool map_eq = map == dev.map().get_data_map();
              std::cerr << dev.map().get("/baz").description << std::endl;
              REQUIRE( !map_eq );
          }
      }
  }
}

#include<coppa/oscquery/json/writer.detail.hpp>
TEST_CASE("effects on map", "[general]") {

  GIVEN( "An empty parameter map" ) {

    for(const auto& file : conformance_test_folders())
    {
        auto base = "conformance/" + file + "/";
        basic_map<ParameterMap> orig_map;
        locked_map<basic_map<ParameterMap>> lm(orig_map);
        remote_map_setter<locked_map<basic_map<ParameterMap>>, osc::sender>
          setter(lm);
        test_remote_device dev(setter, "");

        // Set-up the initial namespace
        dev.message(read_json_file(base + "original.json"));
        auto map1 = coppa::oscquery::json::detail::mapToJson(dev.map().get_data_map(), "/");

        // Apply a command
        dev.message(read_json_file(base + "message.json"));
        auto map2 = coppa::oscquery::json::detail::mapToJson(dev.map().get_data_map(), "/");

        REQUIRE(map2 == read_json_map(base + "expected.json"));
    }
  }
}
