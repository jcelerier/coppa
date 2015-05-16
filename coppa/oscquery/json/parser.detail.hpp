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
{ if(!val) throw BadRequestException{}; }

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

    mapper(Key::attribute<Description>(), p.description, &detail::valToString);
    mapper(Key::attribute<Tags>(),        p.tags,        &detail::jsonToTags);
    mapper(Key::attribute<Access>(),      p.accessmode , &detail::jsonToAccessMode);

    if(obj.find(Key::attribute<Values>()) != obj.end())
    {
      mapper(Key::attribute<Values>(),    p.values,    &detail::jsonToVariantArray);
      mapper(Key::attribute<Ranges>(),     p.ranges,    &detail::jsonToRangeArray);
      mapper(Key::attribute<ClipModes>(),  p.clipmodes, &detail::jsonToClipModeArray);
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
}

}
}
}
