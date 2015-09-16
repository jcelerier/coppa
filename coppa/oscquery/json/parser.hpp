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
      json_assert(obj.get(key::osc_port()).is(val_t::integer));

      return obj.get<int>(key::osc_port());
    }

    static MessageType messageType(const std::string& message)
    {
      using namespace detail;
      const json_map obj{ message };
      if(obj.find(key::osc_port()) != obj.end())
        return MessageType::Device;

      else if(obj.find(key::path_added()) != obj.end())
        return MessageType::PathAdded;
      else if(obj.find(key::path_removed()) != obj.end())
        return MessageType::PathRemoved;
      else if(obj.find(key::path_changed()) != obj.end())
        return MessageType::PathChanged;
      else if(obj.find(key::attributes_changed()) != obj.end())
        return MessageType::AttributesChanged;

      else if(obj.find(key::paths_added()) != obj.end())
        return MessageType::PathsAdded;
      else if(obj.find(key::paths_removed()) != obj.end())
        return MessageType::PathsRemoved;
      else if(obj.find(key::paths_changed()) != obj.end())
        return MessageType::PathsChanged;
      else if(obj.find(key::attributes_changed_array()) != obj.end())
        return MessageType::AttributesChangedArray;

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
      map.merge(parseNamespace<BaseMapType>(obj.get<json_map>(key::path_added())));
    }

    template<typename Map>
    static void path_changed(Map& map, const std::string& message)
    {
      using namespace detail;
      json_map mess{message};

      // Get the object
      const auto& obj = mess.get<json_map>(key::path_changed());

      // 2. Remove the existing path
      map.remove(valToString(obj.get(key::full_path())));

      // 3. Replace it
      readObject(map, obj);
    }

    template<typename Map>
    static void path_removed(Map& map, const std::string& message)
    {
      using namespace detail;
      json_map obj{message};

      const auto& path = valToString(obj.get(key::path_removed()));
      json_assert(map.existing_path(path));
      map.remove(path);
    }

    template<typename Map>
    static void attributes_changed(Map& map, const std::string& message)
    {
      using namespace detail;
      json_map obj{message};
      const auto& attr_changed = obj.get<json_map>(key::attributes_changed());

      // 1. Search for the paths
      for(const auto& path : attr_changed.get_keys())
      {
        json_assert(map.has(path));
        const auto& path_obj = attr_changed.template get<json_map>(path);

        // A lambda used to update the boost map.
        // Here, since we are in attributes_changed, we will just ignore
        // the missing members.
        auto mapper = [&] (const std::string& name, auto&& member, auto&& method)
        {
          if(path_obj.find(name) != path_obj.end())
            map.update(path, [&] (Parameter& p) { p.*member = method(path_obj.template get(name)); });
        };

        // 2. Map the values
        // TODO check that they are correct (and put in detail).
        mapper(key::attribute<Description>(), &Parameter::description, &valToString);
        mapper(key::attribute<Tags>(),        &Parameter::tags,        &jsonToTags);
        mapper(key::attribute<Access>(),      &Parameter::accessmode,  &jsonToAccessMode);
        mapper(key::attribute<Values>(),      &Parameter::values,      &jsonToVariantArray);
        mapper(key::attribute<Ranges>(),      &Parameter::ranges,      &jsonToRangeArray);
        mapper(key::attribute<ClipModes>(),   &Parameter::clipmodes,   &jsonToClipModeArray);
      }
    }


    // Plural forms
    template<typename BaseMapType, typename Map>
    static void paths_added(Map& map, const std::string& message)
    {
      using namespace detail;

      json_map mess{message};
      const auto& arr = detail::valToArray(mess["paths_added"]);
      for(const auto& elt : arr)
      {
        map.merge(parseNamespace<BaseMapType>(elt.as<json_map>()));
      }
    }

    template<typename Map>
    static void paths_changed(Map& map, const std::string& message)
    {
      std::cerr << "TODO" << std::endl;
    }

    template<typename Map>
    static void paths_removed(Map& map, const std::string& message)
    {
      json_map mess{message};
      const auto& arr = detail::valToArray(mess["paths_removed"]);

      for(const auto& elt : arr)
      {
        const auto& path = detail::valToString(elt);
        json_assert(map.existing_path(path));
        map.remove(path);
      }
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

