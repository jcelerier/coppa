#pragma once
#include <coppa/oscquery/json/writer.detail.hpp>
namespace coppa
{
namespace oscquery
{
namespace json
{
class writer
{
  private:
    // The following three methods are here
    // to make the attributes_changed message.
    // End of recursion
    template<typename Attribute>
    static void addAttributes(
        json_map& map,
        const Attribute& attr)
    {
      using namespace detail;
      map[key::attribute(attr)] = attributeToJson(attr);
    }

    template<typename Attribute, typename... Attributes>
    static void addAttributes(
        json_map& map,
        const Attribute& attr,
        Attributes&&... attrs)
    {
      using namespace detail;
      map[attributeToKey(attr)] = attributeToJson(attr);
      addAttributes(std::forward<Attributes>(attrs)...);
    }

  public:
    // Initialisation
    static std::string device_info(int port)
    {
      json_map map;
      map[key::osc_port()] = port;

      return map.to_string();
    }

    // Format interface
    // Queries
    template<typename... Args>
    static std::string query_namespace(
        Args&&... args)
    {
      using namespace detail;
      return mapToJson(std::forward<Args>(args)...).to_string();
    }

    template<typename StringVec_T>
    static std::string query_attributes(
        const Parameter& param,
        const StringVec_T& methods)
    {
      using namespace detail;
      json_map map;
      for(auto& method : methods)
        map[method] = attributeToJsonValue(param, method);
      return map.to_string();
    }


    // Update messages
    template<typename... Args>
    static std::string path_added(
        Args&&... args)
    {
      using namespace detail;
      json_map map;
      map[key::path_added()] = mapToJson(std::forward<Args>(args)...);
      return map.to_string();
    }

    template<typename... Args>
    static std::string path_changed(
        Args&&... args)
    {
      using namespace detail;
      json_map map;
      map[key::path_changed()] = mapToJson(std::forward<Args>(args)...);
      return map.to_string();
    }

    static std::string path_removed(
        const std::string& path)
    {
      using namespace detail;
      json_map map;
      map[key::path_removed()] = path;
      return map.to_string();
    }

  public:
    template<typename... Attributes>
    static std::string attributes_changed(
        const std::string& path,
        Attributes&&... attrs)
    {
      using namespace detail;
      // TODO what if type changed? We have to add the type array.
      json_map objmap;
      objmap[key::full_path()] = path;

      addAttributes(objmap, std::forward<Attributes>(attrs)...);

      json_map map;
      map[key::attributes_changed()] = objmap;
      return map.to_string();
    }


    template<typename Map, typename Vector>
    static std::string paths_added(
        const Map& theMap,
        const Vector& vec)
    {
      using namespace detail;
      json_array arr;
      for(const auto& elt : vec)
      {
        arr.push_back(mapToJson(theMap, elt));
      }

      json_map map;
      map[key::paths_added()] = arr;
      return map.to_string();
    }

    template<typename Map, typename Vector>
    static std::string paths_changed(
        const Map& theMap,
        const Vector& vec)
    {
      using namespace detail;
      json_array arr;
      for(const auto& elt : vec)
      {
        arr.push_back(mapToJson(theMap, elt));
      }

      json_map map;
      map[key::paths_changed()] = arr;
      return map.to_string();
    }

    template<typename Vector>
    static std::string paths_removed(
        const Vector& vec)
    {
      using namespace detail;
      json_array arr;
      for(const auto& elt : vec)
      {
        arr.push_back(elt);
      }

      json_map map;
      map[key::paths_removed()] = arr;
      return map.to_string();
    }

    template<typename... Attributes>
    static std::string attributes_changed_array(
        const std::string& path,
        Attributes&&... attrs)
    {
      using namespace detail;
      // TODO what if type changed? We have to add the type array.
      json_map objmap;
      objmap[key::full_path()] = path;

      addAttributes(objmap, std::forward<Attributes>(attrs)...);

      json_map map;
      map[key::attributes_changed()] = objmap;
      return map.to_string();
    }
};
} // JSON
} // oscquery
} // coppa

