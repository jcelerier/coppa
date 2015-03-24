#pragma once
#include <jeayeson/jeayeson.hpp>
#include <coppa/oscquery/parameter.hpp>
#include <boost/bimap.hpp>
#include <boost/assign.hpp>
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

class JSONRead
{
  public:
    static int getPort(const std::string& message)
    {
      const json_map obj{ message };

      if(obj.get("osc_port").get_type() != json_value::type::integer)
        throw BadRequestException{};

      return obj.get<int>("osc_port");
    }

    static MessageType messageType(const std::string& message)
    {
      const json_map obj{ message };
      if(obj.find("osc_port") != obj.end())
        return MessageType::Device;
      if(obj.find("path_added") != obj.end())
        return MessageType::PathAdded;
      if(obj.find("path_removed") != obj.end())
        return MessageType::PathRemoved;
      if(obj.find("path_changed") != obj.end())
        return MessageType::PathChanged;

      return MessageType::Namespace; // TODO More checks needed
    }

    static ParameterMap toMap(const std::string& message)
    {
      json_map obj{message};
      ParameterMap map;

      try {
        readObject(obj, map);
      }
      catch(BadRequestException& req) {
        throw BadRequestException{message};
      }

      return map;
    }

    static auto jsonToTags(const json_value& val)
    {
      if(val.get_type() != json_value::type::array)
        throw BadRequestException{};

      const auto& arr = val.get<json_array>();
      std::vector<Tag> tags;
      for(const auto& elt : arr)
      {
        if(elt.get_type() != json_value::type::string)
          throw BadRequestException{};

        tags.push_back(elt.get<std::string>());
      }

      return tags;
    }

    static std::string jsonToString(const json_value& val)
    {
      if(val.get_type() != json_value::type::string)
        throw BadRequestException{};

      return val.as<std::string>();
    }

    static auto jsonToAccessMode(const json_value& val)
    {
      if(val.get_type() != json_value::type::integer)
        throw BadRequestException{};

      return  static_cast<AccessMode>(val.as<int>());
    }

    static auto jsonToValueArray(const json_value& val)
    {
      using val_t = json_value::type;

      if(val.get_type() != json_value::type::array)
        throw BadRequestException{};

      const auto& json_vals = val.get<json_array>();
      std::vector<Variant> v;
      for(const auto& val : json_vals)
      {
        switch(val.get_type())
        {
          case val_t::integer: v.push_back(int(val.get<int>())); break;
          case val_t::real:    v.push_back(float(val.get<double>())); break;
          case val_t::string:  v.push_back(val.get<std::string>());break;

          case val_t::boolean:
          default:
            break;
            // TODO blob
        }
      }

      return v;
    }

    static auto jsonToClipModeArray(const json_value& val)
    {
      static const boost::bimap<std::string, ClipMode> clipmodeMap =
          boost::assign::list_of<boost::bimap<std::string, ClipMode>::relation>
         ("None", ClipMode::None)
         ("Low", ClipMode::Low)
         ("High", ClipMode::High)
         ("Both", ClipMode::Both);

      if(val.get_type() != json_value::type::array)
        throw BadRequestException{};

      const auto& json_clipmodes = val.get<json_array>();
      std::vector<ClipMode> vec;
      for(const json_value& value : json_clipmodes)
      {
        if(value.get_type() != json_value::type::string)
          throw BadRequestException{};

        auto it = clipmodeMap.left.find(value.get<std::string>());
        if(it == end(clipmodeMap.left))
          throw BadRequestException{};

        vec.push_back(it->second);
      }

      return vec;
    }

    static auto jsonToRangeArray(const json_value& val)
    {
      if(val.get_type() != json_value::type::array)
        throw BadRequestException{};

      const auto& json_ranges = val.get<json_array>();
      std::vector<Range> ranges;
      auto jsonValueToVariant = [] (const json_value& aValue)
      {
        switch(aValue.get_type())
        {
          case json_value::type::integer: return Variant{int(aValue.get<int>())};
          case json_value::type::real:    return Variant{float(aValue.get<float>())};
          case json_value::type::boolean: return Variant{bool(aValue.get<bool>())};
          case json_value::type::string:  return Variant{aValue.get<std::string>()};
          case json_value::type::null:    return Variant{};
          default: throw BadRequestException{};
            // TODO blob
        }
      };

      for(const json_value& range_val : json_ranges)
      {
        if(range_val.get_type() != json_value::type::array)
          throw BadRequestException{};

        json_array range_arr = range_val.as<json_array>();
        if(range_arr.size() != 3)
          throw BadRequestException{};

        Range range;
        range.min = jsonValueToVariant(range_arr.get(0));
        range.max = jsonValueToVariant(range_arr.get(1));

        if(range_arr.get(2).get_type() == json_value::type::array)
        {
          for(const json_value& enum_val : range_arr.get<json_array>(2))
            range.values.push_back(jsonValueToVariant(enum_val));
        }
        else if(range_arr.get(2).get_type() != json_value::type::null)
        {
          throw BadRequestException{};
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

        p.destination = jsonToString(obj.get("full_path"));

        if(obj.find("description") != obj.end())
          p.description = jsonToString(obj.get("description"));

        if(obj.find("tags") != obj.end())
          p.tags = jsonToTags(obj.get("tags"));

        if(obj.find("access") != obj.end())
          p.accessmode = jsonToAccessMode(obj.get("access"));

        if(obj.find("value") != obj.end())
        {
          p.values = jsonToValueArray(obj.get("value"));
          p.ranges = jsonToRangeArray(obj.get("range"));
          p.clipmodes = jsonToClipModeArray(obj.get("clipmode"));
        }

        map.insert(p);
      }

      // Recurse on the children
      if(obj.find("contents") != obj.end())
      {
        // contents is a json_map where each child is a key / json_map
        if(obj.get("contents").get_type() != json_value::type::map)
          throw BadRequestException{};

        json_map contents = obj.get<json_map>("contents");
        for(auto key : contents.get_keys())
        {
          if(contents.get(key).get_type() != json_value::type::map)
            throw BadRequestException{};

          readObject(contents.get<json_map>(key), map);
        }
      }
    }
};


class JSONFormat
{
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
