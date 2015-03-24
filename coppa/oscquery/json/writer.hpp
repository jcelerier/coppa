#pragma once
#include <jeayeson/jeayeson.hpp>
#include <coppa/oscquery/parameter.hpp>
#include <boost/tokenizer.hpp>
#include <boost/bimap.hpp>
#include <boost/assign.hpp>
namespace coppa
{
namespace oscquery
{
class JSONFormat
{
    using val_t = json_value::type;
  public:
    // Format interface
    template<typename... Args>
    static std::string marshallParameterMap(Args&&... args)
    {
      return mapToJson(std::forward<Args>(args)...).to_string();
    }

    template<typename... Args>
    static std::string marshallAttribute(Args&&... args)
    {
      return attributeToJson(std::forward<Args>(args)...).to_string();
    }

    static std::string insertParameterMessage(const Parameter&)
    {
      return {};
    }

    static std::string removePathMessage(const std::string& )
    {
      return {};
    }

    static std::string deviceInfo(int port)
    {
      json_map map;
      map["osc_port"] = port;

      return map.to_string();
    }

    template<typename Attribute>
    static std::string attributeChangedMessage(const std::string& path, const Attribute& attr)
    {
      // TODO what if type changed?
      json_map map;
      map["path_changed"] = path;
      map[attributeToKey(attr)] = attributeToJson(attr);
      return map.to_string();
    }

  private:
    static constexpr const char* attributeToKey(const Values& ) { return "value"; }
    static constexpr const char* attributeToKey(const Ranges& ) { return "range"; }
    static constexpr const char* attributeToKey(const ClipModes& ) { return "clipmode"; }
    static constexpr const char* attributeToKey(const Access& ) { return "access"; }
    static constexpr const char* attributeToKey(const Description& ) { return "description"; }
    static constexpr const char* attributeToKey(const Tags& ) { return "tags"; }

    static auto attributeToJson(const Values& val) { return getJsonValueArray(val); }
    static auto attributeToJson(const Ranges& val) { return getJsonRangeArray(val); }
    static auto attributeToJson(const ClipModes& val) { return getJsonClipModeArray(val); }
    static auto attributeToJson(const Access& val) { return static_cast<int>(val.accessmode); }
    static auto attributeToJson(const Description& val) { return val.description; }
    static auto attributeToJson(const Tags& val) { return getJsonTags(val); }

    static std::string getJsonTypeString(const Parameter& parameter)
    {
      std::string str_type;
      for(const auto& value : parameter.values)
      {
        switch(value.which())
        {
          case 0: str_type += "i"; break;
          case 1: str_type += "f"; break;
            // case 2: str_type += "B"; break; -> no bool
          case 3: str_type += "s"; break;
          case 4: str_type += "b"; break;
        }
      }

      return str_type;
    }

    static void addValueToJsonArray(json_array& array, const Variant& val)
    {
      using namespace eggs::variants;
      switch(val.which())
      {
        case 0: array.add(get<int>(val)); break;
        case 1: array.add(get<float>(val)); break;
          //case 2: array.add(get<bool>(val)); break;
        case 3: array.add(get<std::string>(val)); break;
        case 4: array.add(get<const char*>(val)); break;
      }
    }

    static json_array getJsonValueArray(const Values& values)
    {
      json_array value_arr;
      for(const auto& value : values.values)
      {
        addValueToJsonArray(value_arr, value);
      }

      return value_arr;
    }

    static json_array getJsonClipModeArray(const ClipModes& clipmodes)
    {
      static const boost::bimap<std::string, ClipMode> clipmodeMap =
          boost::assign::list_of<boost::bimap<std::string, ClipMode>::relation>
          ("None", ClipMode::None)
          ("Low", ClipMode::Low)
          ("High", ClipMode::High)
          ("Both", ClipMode::Both);

      json_array clip_arr;
      for(const auto& clipmode : clipmodes.clipmodes)
      {
        clip_arr.add(clipmodeMap.right.at(clipmode));
      }

      return clip_arr;
    }

    static json_array getJsonRangeArray(const Ranges& ranges)
    {
      json_array range_arr;
      for(const auto& range : ranges.ranges)
      {
        json_array range_subarray;
        if(!range.min)
        { range_subarray.add(json_value::null_t{}); }
        else
        { addValueToJsonArray(range_subarray, *range.min); }

        if(!range.max)
        { range_subarray.add(json_value::null_t{}); }
        else
        { addValueToJsonArray(range_subarray, *range.max); }

        if(range.values.empty())
        { range_subarray.add(json_value::null_t{}); }
        else
        {
          json_array range_values_array;
          for(auto& elt : range.values)
          {
            addValueToJsonArray(range_values_array, elt);
          }
          range_subarray.add(range_values_array);
        }

        range_arr.add(range_subarray);
      }

      return range_arr;
    }

    static json_array getJsonTags(const Tags& tags)
    {
      json_array arr;
      for(const auto& tag : tags.tags)
        arr.add(tag);

      return arr;
    }

    static json_map attributeToJson(const Parameter& parameter, std::string method)
    {
      json_map map;

      if(method == "value")
      { map.set("value", getJsonValueArray(parameter)); }
      else if(method == "range")
      { map.set("range", getJsonRangeArray(parameter)); }
      else if(method == "clipmode")
      { map.set("clipmode", getJsonClipModeArray(parameter)); }
      else if(method == "access")
      { map.set("access", static_cast<int>(parameter.accessmode)); }
      else if(method == "type")
      { map.set("type", getJsonTypeString(parameter)); }
      else if(method == "description")
      { map.set("description", parameter.description); }
      else if(method == "tags")
      { map.set("tags", getJsonTags(parameter)); }
      else if(method == "full_path")
      { map.set("full_path", parameter.destination); }

      return map;
    }

    static void parameterToJson(const Parameter& parameter,
                                json_map& obj)
    {
      using namespace std;
      using namespace boost;
      using namespace eggs::variants;

      // These attributes are always here
      obj.set("full_path", parameter.destination);
      obj.set("access", static_cast<int>(parameter.accessmode));

      // Potentially empty attributes :
      // Description
      if(!parameter.description.empty())
      {
        obj.set("description", parameter.description);
      }

      // Tags
      if(!parameter.tags.empty())
      {
        obj.set("tags", getJsonTags(parameter));
      }

      // Handling of the types / values
      if(!parameter.values.empty())
      {
        obj.set("type", getJsonTypeString(parameter));
        obj.set("value", getJsonValueArray(parameter));
        obj.set("clipmode", getJsonClipModeArray(parameter));
        obj.set("range", getJsonRangeArray(parameter));
      }
    }

    // A ParameterMap can be JSON'd
    static json_map mapToJson(const ParameterMap& map, std::string root)
    {
      using namespace std;
      using namespace boost;
      using namespace eggs::variants;
      // Root node
      json_map localroot;

      // Create a tree with the parameters
      for(const auto& parameter : filter(map, root))
      {
        // Trunk the given root from the parameters
        auto trunked_dest = parameter.destination;
        if(root != "/")
          trunked_dest.erase(0, root.length());

        char_separator<char> sep("/");
        tokenizer<char_separator<char>> tokens(trunked_dest, sep);

        // Create the required parts of the tree and navigate to the corresponding node
        auto* current_map = &localroot;
        for(const auto& token : tokens)
        {
          if(!current_map->has("contents"))
          { current_map->set("contents", json_map{}); }

          current_map = &current_map->get_for_path<json_map>("contents");

          if(!current_map->has(token))
          { current_map->set(token, json_map{}); }

          current_map = &current_map->get_for_path<json_map>(token);
        }

        parameterToJson(parameter, *current_map);
      }

      return localroot;
    }
};
}
}