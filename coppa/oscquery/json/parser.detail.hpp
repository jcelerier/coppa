#pragma once
#include <coppa/exceptions/BadRequest.hpp>
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
namespace JSON
{
using val_t = json_value::type;

static void json_assert(bool val)
{
    if(!val)
        throw BadRequestException{};
}

namespace detail
{
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
    const auto& range_arr = valToArray(range_val);
    if(range_arr.size() != 3)
      throw BadRequestException{};

    Range range;
    range.min = jsonToVariant(range_arr.get(0));
    range.max = jsonToVariant(range_arr.get(1));
    const auto& thirdElement = range_arr.get(2);
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

static auto jsonToTypeVector(const json_value& val)
{
  std::vector<std::size_t> types_vec;
  for(const auto& c : val.get<std::string>())
  {
    switch(c)
    {
      case 'i':
        types_vec.push_back(0);
        break;
      case 'f':
        types_vec.push_back(1);
        break;
      case 's':
        types_vec.push_back(3);
        break;
      default:
        throw BadRequestException("Unsupported type");
    }
  }

  return types_vec;
}

template<typename Map>
static void readObject(Map& map, const json_map& obj)
{
  // If it's a real parameter
  if(obj.find(key::full_path()) != obj.end())
  {
    Parameter p;
    p.destination = valToString(obj.get(key::full_path()));

    // To map non-mandatory elements
    auto mapper = [&] (const std::string& name, auto& member, auto&& method)
    {
      auto it = obj.find(name);
      if(it != obj.end())
        member = method(it->second);
    };

    // Note : clipmode : software that expects it should assume that no clipping will be performed
    // What about integer clipping ? should we use arbitrary precision math ??
    mapper(key::attribute<Description>(), p.description, &detail::valToString);
    mapper(key::attribute<Tags>(),        p.tags,        &detail::jsonToTags);
    mapper(key::attribute<Access>(),      p.accessmode , &detail::jsonToAccessMode);

    // Types
    auto type_it = obj.find(key::type());
    if(type_it != obj.end())
    {
      auto type_vec = jsonToTypeVector(type_it->second);

      auto val_it = obj.find(key::attribute<Values>());
      // If there are values in the json
      if(val_it != obj.end())
      {
        p.values = detail::jsonToVariantArray(val_it->second);

        // Check that the types are correct
        json_assert(p.values.size() == type_vec.size());
        for(int i = 0; i < p.values.size(); i++)
        {
          // Bad parse of a float as an
          if(type_vec[i] == 1 && p.values[i].which() == 0)
          {
             p.values[i] = float(eggs::variants::get<int>(p.values[i]));
          }

          json_assert(p.values[i].which() == type_vec[i]);
        }
      }
      else
      {
        // Make a default Values/range/clipmode array
        for(int i = 0; i < type_vec.size(); i++)
          addDefaultValue(p, type_vec[i]);
      }

      auto range_it = obj.find(key::attribute<Ranges>());
      if(range_it != obj.end())
      {
        p.ranges = jsonToRangeArray(range_it->second);

        json_assert(p.ranges.size() == type_vec.size());
        for(int i = 0; i < p.ranges.size(); i++)
        {
          static auto invalid = Variant().which();
          const auto& elt = p.ranges[i];
          json_assert(elt.min.which() == invalid || elt.min.which() == type_vec[i]);
          json_assert(elt.max.which() == invalid || elt.max.which() == type_vec[i]);
          for(const auto& range_elt : elt.values)
          {
            json_assert(range_elt.which() == invalid || range_elt.which() == type_vec[i]);
          }
        }
      }

      auto clipmode_it = obj.find(key::attribute<ClipModes>());
      if(clipmode_it != obj.end())
      {
        p.clipmodes = jsonToClipModeArray(clipmode_it->second);

        json_assert(p.clipmodes.size() == type_vec.size());
      }
    }

    map.add(p);
  }

  // Recurse on the children
  if(obj.find(key::contents()) != obj.end())
  {
    // contents is a json_map where each child is a key / json_map
    for(const auto& val : valToMap(obj.get(key::contents())).get_values())
    {
      readObject(map, valToMap(val));
    }
  }
}
}

}
}
}
