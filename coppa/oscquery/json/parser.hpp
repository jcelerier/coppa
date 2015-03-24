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
      return static_cast<Access::Mode>(valToInt(val));
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
}
}
