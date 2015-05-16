#pragma once
#include <coppa/oscquery/parameter.hpp>
#include <coppa/device/messagetype.hpp>
#include <coppa/oscquery/json/keys.hpp>

#include <jeayeson/jeayeson.hpp>
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

class JSONParser
{
    using val_t = json_value::type;

    static void json_assert(bool val)
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
        default:
          throw BadRequestException{};
          // TODO blob
      }
    }

    static auto jsonToVariantArray(const json_value& json_val)
    {
      std::vector<Variant> v;

      for(const auto& val : valToArray(json_val))
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


    template<typename Map>
    static void readObject(Map& map, const json_map& obj)
    {
      // If it's a real parameter
      if(obj.find(Key::full_path()) != obj.end())
      {
        Parameter p;
        p.destination = valToString(obj.get(Key::full_path()));

        auto mapper = [&] (const std::string& name, auto& member, auto&& method)
        {
          if(obj.find(name) != obj.end()) member = method(obj.get(name));
        };

        mapper(Key::attribute<Description>(), p.description, &JSONParser::valToString);
        mapper(Key::attribute<Tags>(),        p.tags,        &JSONParser::jsonToTags);
        mapper(Key::attribute<Access>(),      p.accessmode , &JSONParser::jsonToAccessMode);

        if(obj.find(Key::attribute<Values>()) != obj.end())
        {
          mapper(Key::attribute<Values>(),    p.values,    &JSONParser::jsonToVariantArray);
          mapper(Key::attribute<Ranges>(),     p.ranges,    &JSONParser::jsonToRangeArray);
          mapper(Key::attribute<ClipModes>(),  p.clipmodes, &JSONParser::jsonToClipModeArray);
        }

        map.add(p);
      }

      // Recurse on the children
      if(obj.find(Key::contents()) != obj.end())
      {
        // contents is a json_map where each child is a key / json_map
        for(const auto& val : valToMap(obj.get(Key::contents())).get_values())
        {
          readObject(map, valToMap(val));
        }
      }
    }

  public:
    static int getPort(const std::string& message)
    {
      const json_map obj{ message };
      json_assert(obj.get(Key::osc_port()).is(val_t::integer));

      return obj.get<int>(Key::osc_port());
    }

    static MessageType messageType(const std::string& message)
    {
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
      Map map;

      readObject(map, obj);

      return map;
    }

    template<typename Map>
    static auto parseNamespace(const std::string& message)
    {
        return parseNamespace<Map>(json_map{message});
    }


    template<typename BaseMapType, typename Map>
    static void parsePathAdded(Map& map, const std::string& message)
    {
      json_map obj{message};
      map.merge(parseNamespace<BaseMapType>(obj.get<json_map>(Key::path_added())));
    }

    template<typename Map>
    static void parsePathChanged(Map& map, const std::string& message)
    {
      // TODO test this
      std::cout << std::endl << "PathChanged" << message << std::endl;
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
    static void parsePathRemoved(Map& map, const std::string& message)
    {
      json_map obj{message};

      auto path = JSONParser::valToString(obj.get(Key::path_removed()));
      json_assert(map.has(path));
      map.remove(path);
    }

    template<typename Map>
    static void parseAttributesChanged(Map& map, const std::string& message)
    {
      // TODO test this
      std::cout << std::endl << "AttributeChanged" << message << std::endl;
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
        mapper(Key::attribute<Description>(), &Parameter::description, &JSONParser::valToString);
        mapper(Key::attribute<Tags>(),        &Parameter::tags,        &JSONParser::jsonToTags);
        mapper(Key::attribute<Access>(),      &Parameter::accessmode,  &JSONParser::jsonToAccessMode);
        mapper(Key::attribute<Values>(),      &Parameter::values,      &JSONParser::jsonToVariantArray);
        mapper(Key::attribute<Ranges>(),      &Parameter::ranges,      &JSONParser::jsonToRangeArray);
        mapper(Key::attribute<ClipModes>(),   &Parameter::clipmodes,   &JSONParser::jsonToClipModeArray);
      }
    }


};
}
}
