#pragma once
#include <coppa/oscquery/json/parser.detail.hpp>

namespace coppa
{
namespace oscquery
{
namespace JSON
{
using val_t = json_value::type;
class parser
{
  public:
    static int getPort(const std::string& message)
    {
      using namespace detail;
      const json_map obj{ message };
      json_assert(obj.get(Key::osc_port()).is(val_t::integer));

      return obj.get<int>(Key::osc_port());
    }

    static MessageType messageType(const std::string& message)
    {
      using namespace detail;
      const json_map obj{ message };
      if(obj.find(Key::osc_port()) != obj.end())     return MessageType::Device;

      else if(obj.find(Key::path_added()) != obj.end())   return MessageType::PathAdded;
      else if(obj.find(Key::path_removed()) != obj.end()) return MessageType::PathRemoved;
      else if(obj.find(Key::path_changed()) != obj.end()) return MessageType::PathChanged;
      else if(obj.find(Key::attributes_changed()) != obj.end()) return MessageType::AttributesChanged;

      else if(obj.find(Key::paths_added()) != obj.end())   return MessageType::PathsAdded;
      else if(obj.find(Key::paths_removed()) != obj.end()) return MessageType::PathsRemoved;
      else if(obj.find(Key::paths_changed()) != obj.end()) return MessageType::PathsChanged;
      else if(obj.find(Key::attributes_changed_array()) != obj.end()) return MessageType::AttributesChangedArray;

      else return MessageType::Namespace; // TODO More checks needed
    }

    template<typename Map>
    static auto parseNamespace(const json_map& obj)
    {
      using namespace detail;
      Map map;

      readObject(map, obj);

      return map;
    }

    template<typename Map>
    static auto parseNamespace(const std::string& message)
    {
      using namespace detail;
      return parseNamespace<Map>(json_map{message});
    }


    template<typename BaseMapType, typename Map>
    static void path_added(Map& map, const std::string& message)
    {
      using namespace detail;
      json_map obj{message};
      map.merge(parseNamespace<BaseMapType>(obj.get<json_map>(Key::path_added())));
    }

    template<typename Map>
    static void path_changed(Map& map, const std::string& message)
    {
      using namespace detail;
      json_map obj{message};
      auto path_changed = obj.get<json_map>(Key::path_changed());

      // 1. Search for the paths
      for(const auto& path : path_changed.get_keys())
      {
        json_assert(map.has(path));
        auto path_obj = path_changed.get<json_map>(path);

        // 2. Remove the existing path
        map.remove(valToString(path_obj.get(Key::full_path())));

        // 3. Replace it
        readObject(map, path_obj);
      }
    }

    template<typename Map>
    static void path_removed(Map& map, const std::string& message)
    {
      using namespace detail;
      json_map obj{message};

      auto path = valToString(obj.get(Key::path_removed()));
      json_assert(map.existing_path(path));
      map.remove(path);
    }

    template<typename Map>
    static void attributes_changed(Map& map, const std::string& message)
    {
      using namespace detail;
      json_map obj{message};
      auto attr_changed = obj.get<json_map>(Key::attributes_changed());

      // 1. Search for the paths
      for(const auto& path : attr_changed.get_keys())
      {
        json_assert(map.has(path));
        auto path_obj = attr_changed.get<json_map>(path);

        // A lambda used to update the boost map.
        // Here, since we are in attributes_changed, we will just ignore
        // the missing members.
        auto mapper = [&] (const std::string& name, auto&& member, auto&& method)
        {
          if(path_obj.find(name) != path_obj.end())
            map.update(path, [&] (Parameter& p) { p.*member = method(path_obj.get(name)); });
        };

        // 2. Map the values
        mapper(Key::attribute<Description>(), &Parameter::description, &valToString);
        mapper(Key::attribute<Tags>(),        &Parameter::tags,        &jsonToTags);
        mapper(Key::attribute<Access>(),      &Parameter::accessmode,  &jsonToAccessMode);
        mapper(Key::attribute<Values>(),      &Parameter::values,      &jsonToVariantArray);
        mapper(Key::attribute<Ranges>(),      &Parameter::ranges,      &jsonToRangeArray);
        mapper(Key::attribute<ClipModes>(),   &Parameter::clipmodes,   &jsonToClipModeArray);
      }
    }


    // Plural forms
    template<typename BaseMapType, typename Map>
    static void paths_added(Map& map, const std::string& message)
    {
      std::cerr << "TODO" << std::endl;
    }

    template<typename Map>
    static void paths_changed(Map& map, const std::string& message)
    {
      std::cerr << "TODO" << std::endl;
    }

    template<typename Map>
    static void paths_removed(Map& map, const std::string& message)
    {
      std::cerr << "TODO" << std::endl;
    }

    template<typename Map>
    static void attributes_changed_array(Map& map, const std::string& message)
    {
      std::cerr << "TODO" << std::endl;
    }
};
}
}
}

