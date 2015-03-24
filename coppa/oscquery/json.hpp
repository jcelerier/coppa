#pragma once
#include <jeayeson/jeayeson.hpp>
#include <coppa/oscquery/parameter.hpp>
#include <boost/bimap.hpp>
#include <boost/assign.hpp>
#include <mutex>
namespace coppa
{
namespace oscquery
{
class BadRequestException: public std::domain_error
{
  public:
    BadRequestException():
      std::domain_error{"Bad request"} { }

    BadRequestException(const std::string& message):
      std::domain_error{"Bad request : " + message} { }
};

enum class MessageType
{
  Namespace, Device, PathChanged, PathAdded, PathRemoved
};

class JSONParser
{
    using val_t = json_value::type;

    static constexpr void json_assert(bool val)
    { if(!val) throw BadRequestException{}; }

    static const auto& valToString(const json_value& val)
    {
      json_assert(val.is(val_t::string));
      return val.as<std::string>();
    }
    static const auto& valToArray(const json_value& val)
    {
      json_assert(val.is(val_t::array));
      return val.as<json_array>();
    }
    static const auto& valToMap(const json_value& val)
    {
      json_assert(val.is(val_t::map));
      return val.as<json_map>();
    }
    static int valToInt(const json_value& val)
    {
      json_assert(val.is(val_t::integer));
      return val.as<int>();
    }

    static auto jsonToTags(const json_value& val)
    {
      std::vector<Tag> tags;
      for(const auto& elt : valToArray(val))
        tags.push_back(valToString(elt));

      return tags;
    }

    static auto jsonToAccessMode(const json_value& val)
    {
      return static_cast<AccessMode>(valToInt(val));
    }

    static auto jsonToVariant(const json_value& val)
    {
      switch(val.get_type())
      {
        case val_t::integer: return Variant{int(val.get<int>())};
        case val_t::real:    return Variant{float(val.get<float>())};
        case val_t::boolean: return Variant{bool(val.get<bool>())};
        case val_t::string:  return Variant{val.get<std::string>()};
        case val_t::null:    return Variant{};
        default: throw BadRequestException{};
          // TODO blob
      }
    }

    static auto jsonToVariantArray(const json_value& val)
    {
      std::vector<Variant> v;
      for(const auto& val : valToArray(val))
        v.push_back(jsonToVariant(val));

      // TODO : error-checking with the "types"

      return v;
    }

    static auto jsonToClipModeArray(const json_value& val)
    {
      static const boost::bimap<std::string, ClipMode> clipmodeMap =
          boost::assign::list_of<boost::bimap<std::string, ClipMode>::relation>
         ("None", ClipMode::None)
         ("Low",  ClipMode::Low)
         ("High", ClipMode::High)
         ("Both", ClipMode::Both);

      std::vector<ClipMode> vec;
      for(const json_value& value : valToArray(val))
      {
        auto it = clipmodeMap.left.find(valToString(value));
        if(it == end(clipmodeMap.left))
          throw BadRequestException{};

        vec.push_back(it->second);
      }

      return vec;
    }

    static auto jsonToRangeArray(const json_value& val)
    {
      std::vector<Range> ranges;
      for(const json_value& range_val : valToArray(val))
      {
        auto range_arr = valToArray(range_val);
        if(range_arr.size() != 3)
          throw BadRequestException{};

        Range range;
        range.min = jsonToVariant(range_arr.get(0));
        range.max = jsonToVariant(range_arr.get(1));
        auto thirdElement = range_arr.get(2);
        if(thirdElement.is(val_t::array))
        {
          for(const auto& enum_val : valToArray(thirdElement))
            range.values.push_back(jsonToVariant(enum_val));
        }
        else
        {
          json_assert(range_arr.get(2).is(val_t::null));
        }

        ranges.push_back(range);
      }

      return ranges;
    }


    static void readObject(const json_map& obj, ParameterMap& map)
    {
      // If it's a real parameter
      if(obj.find("full_path") != obj.end())
      {
        Parameter p;
        p.destination = valToString(obj.get("full_path"));

        auto mapper = [&] (const std::string& name, auto& member, auto&& method)
        {
          if(obj.find(name) != obj.end()) member = method(obj.get(name));
        };

        mapper("description", p.description, &JSONParser::valToString);
        mapper("tags",        p.tags,        &JSONParser::jsonToTags);
        mapper("access",      p.accessmode , &JSONParser::jsonToAccessMode);

        if(obj.find("value") != obj.end())
        {
          mapper("value",    p.values,    &JSONParser::jsonToVariantArray);
          mapper("range",    p.ranges,    &JSONParser::jsonToRangeArray);
          mapper("clipmode", p.clipmodes, &JSONParser::jsonToClipModeArray);
        }

        map.insert(p);
      }

      // Recurse on the children
      if(obj.find("contents") != obj.end())
      {
        // contents is a json_map where each child is a key / json_map
        for(const auto& val : valToMap(obj.get("contents")).get_values())
        {
          readObject(valToMap(val), map);
        }
      }
    }

  public:
    static int getPort(const std::string& message)
    {
      const json_map obj{ message };
      json_assert(obj.get("osc_port").is(val_t::integer));

      return obj.get<int>("osc_port");
    }

    static MessageType messageType(const std::string& message)
    {
      const json_map obj{ message };
      if(obj.find("osc_port") != obj.end())     return MessageType::Device;
      if(obj.find("path_added") != obj.end())   return MessageType::PathAdded;
      if(obj.find("path_removed") != obj.end()) return MessageType::PathRemoved;
      if(obj.find("path_changed") != obj.end()) return MessageType::PathChanged;

      return MessageType::Namespace; // TODO More checks needed
    }

    template<typename Map>
    static auto parseNamespace(const std::string& message)
    {
      json_map obj{message};
      Map map;

      try {
        readObject(obj, map);
      }
      catch(BadRequestException& req) {
        throw BadRequestException{message};
      }

      return map;
    }

    template<typename Map>
    static void parsePathChanged(Map& map, const std::string& message)
    {
      json_map obj{message};
      std::string path = JSONParser::valToString(obj.get("path_changed"));
      auto getter = [] (auto&& member) { return [&] (auto&& p) -> auto& { return p.*member; }; };
      auto mapper = [&] (const std::string& name, auto&& getter, auto&& method)
      {
        if(obj.find(name) != obj.end())
          map.update(path, [&] (Parameter& p) { getter(p) = method(obj.get(name)); });
      };

      mapper("description", getter(&Parameter::description), &JSONParser::valToString);
      mapper("tags",        getter(&Parameter::tags),        &JSONParser::jsonToTags);
      mapper("access",      getter(&Parameter::accessmode),  &JSONParser::jsonToAccessMode);
      mapper("value",       getter(&Parameter::values),      &JSONParser::jsonToVariantArray);
      mapper("range",       getter(&Parameter::ranges),      &JSONParser::jsonToRangeArray);
      mapper("clipmode",    getter(&Parameter::clipmodes),   &JSONParser::jsonToClipModeArray);
    }
};


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
