#pragma once
#include <coppa/exceptions/BadRequest.hpp>
#include <coppa/oscquery/parameter.hpp>
#include <coppa/device/messagetype.hpp>
#include <coppa/oscquery/json/keys.hpp>

#include <base64/base64.h>

#include <jeayeson/jeayeson.hpp>
#include <boost/bimap.hpp>
#include <boost/assign.hpp>

namespace coppa
{
namespace oscquery
{
namespace json
{
// TODO make global tests with three files: a given tree, a transformation, and an expected result.
using val_t = json_value::type;

inline void json_assert(bool val)
{
    if(!val)
        throw BadRequestException{};
}

namespace detail
{
inline const auto& valToString(const json_value& val)
{
  json_assert(val.is(val_t::string));
  return val.as<std::string>();
}
inline const auto& valToArray(const json_value& val)
{
  json_assert(val.is(val_t::array));
  return val.as<json_array>();
}
inline const auto& valToMap(const json_value& val)
{
  json_assert(val.is(val_t::map));
  return val.as<json_map>();
}
inline int valToInt(const json_value& val)
{
  json_assert(val.is(val_t::integer));
  return val.as<int>();
}

inline auto jsonToTags(const json_value& val)
{
  std::vector<Tag> tags;
  for(const auto& elt : valToArray(val))
    tags.push_back(valToString(elt));

  return tags;
}

inline auto jsonToAccessMode(const json_value& val)
{
  return static_cast<Access::Mode>(valToInt(val));
}

inline auto jsonToVariant(const json_value& val)
{
  switch(val.get_type())
  {
    case val_t::integer: return Variant{int(val.get<int>())};
    case val_t::real:    return Variant{float(val.get<float>())};
    case val_t::boolean: return Variant{bool(val.get<bool>())};
    case val_t::string:  return Variant{val.get<std::string>()};
    // Note : this may match the blob, since it is saved as a string, too.
    case val_t::null:    return Variant{};
    default:
      throw BadRequestException{};
  }
}

inline auto jsonToVariant_checked(const json_value& val, std::size_t type)
{
  // TODO more checks.
  switch(val.get_type())
  {
    case val_t::integer:
    {
      // This case is special because we can't infer if 10 is 10 or 10.0 in json
      if(type == 1)
        return Variant{float(val.get<int>())};
      else if(type == 0)
        return Variant{int(val.get<int>())};
      else
        throw BadRequestException{};
    }
    case val_t::real:
    {
      json_assert(type == 1);
      return Variant{float(val.get<float>())};
    }
    case val_t::boolean:
    {
      json_assert(type == 2);
      return Variant{bool(val.get<bool>())};
    }
    case val_t::string:
    {
      // Here we can't infer from the json if
      // it's a plain string or a blob.
      if(type == 3)
      {
        return Variant{val.get<std::string>()};
      }
      else if(type == 4)
      {
        std::string out;
        bool b = Base64::Decode(val.get<std::string>(), &out);
        assert(b);
        return Variant{coppa::Generic{out}};
      }
      else
      {
        throw BadRequestException{};
      }
    }
    case val_t::null:    return Variant{};
    default:
      throw BadRequestException{};
  }
}


inline auto jsonToVariantArray(const json_value& json_val)
{
  vector<Variant> v;

  for(const auto& val : valToArray(json_val))
    v.push_back(jsonToVariant(val));

  return v;
}


inline auto jsonToVariantArray_checked(
    const json_value& json_val,
    const vector<std::size_t>& type_vec)
{
  vector<Variant> v;

  auto arr = valToArray(json_val);

  json_assert(arr.size() == type_vec.size());
  for(int i = 0; i < arr.size(); i++)
  {
    v.push_back(jsonToVariant_checked(arr[i], type_vec[i]));
  }

  return v;
}

inline auto jsonToClipModeArray(const json_value& val)
{
  static const boost::bimap<std::string, ClipMode> clipmodeMap =
      boost::assign::list_of<boost::bimap<std::string, ClipMode>::relation>
      ("None", ClipMode::None)
      ("Low",  ClipMode::Low)
      ("High", ClipMode::High)
      ("Both", ClipMode::Both);

  vector<ClipMode> vec;
  for(const json_value& value : valToArray(val))
  {
    auto it = clipmodeMap.left.find(valToString(value));
    if(it == end(clipmodeMap.left))
      throw BadRequestException{};

    vec.push_back(it->second);
  }

  return vec;
}

inline auto jsonToRangeArray(const json_value& val)
{
  vector<Range> ranges;

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
      {
        range.values.push_back(jsonToVariant(enum_val));
      }
    }
    else
    {
      json_assert(range_arr.get(2).is(val_t::null));
    }

    ranges.push_back(range);
  }

  return ranges;
}

inline auto jsonToRangeArray_checked(
    const json_value& val, 
    const vector<std::size_t>& type_vec)
{
  vector<Range> ranges;

  int i = 0;
  for(const json_value& range_val : valToArray(val))
  {
    const auto& range_arr = valToArray(range_val);
    if(range_arr.size() != 3)
      throw BadRequestException{};

    auto current_type = type_vec[i];
    Range range;
    range.min = jsonToVariant_checked(range_arr.get(0), current_type);
    range.max = jsonToVariant_checked(range_arr.get(1), current_type);

    const auto& thirdElement = range_arr.get(2);
    if(thirdElement.is(val_t::array))
    {
      for(const auto& enum_val : valToArray(thirdElement))
      {
        range.values.push_back(jsonToVariant_checked(enum_val, current_type));
      }
    }
    else
    {
      json_assert(range_arr.get(2).is(val_t::null));
    }

    ranges.push_back(range);
    i++;
  }

  return ranges;
}

static std::size_t OSCToVariantType(char c)
{
  switch(c)
  {
    case 'i': return 0;
    case 'f': return 1;
    case 'T': return 2;
    case 'F': return 2;
    case 's': return 3;
    case 'b': return 4;
    default: throw BadRequestException("Unsupported type");
  }
}

inline auto jsonToTypeVector(const json_value& val)
{
  auto str = val.get<std::string>();
  vector<std::size_t> types_vec(str.size());
  std::transform(str.begin(), str.end(), types_vec.begin(), &OSCToVariantType);

  return types_vec;
}

template<typename Map>
void readObject(Map& map, const json_map& obj)
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
    mapper(key::attribute<Access>(),      p.access , &detail::jsonToAccessMode);

    // Types
    auto type_it = obj.find(key::type());
    if(type_it != obj.end())
    {
      auto type_vec = jsonToTypeVector(type_it->second);

      auto val_it = obj.find(key::attribute<Values>());
      // If there are values in the json
      if(val_it != obj.end())
      {
        p.values = detail::jsonToVariantArray_checked(val_it->second, type_vec);
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
        p.ranges = jsonToRangeArray_checked(range_it->second, type_vec);

        json_assert(p.ranges.size() == type_vec.size());
        for(int i = 0; i < p.ranges.size(); i++)
        {
          const auto& elt = p.ranges[i];
          json_assert(elt.min.which() == elt.min.npos || elt.min.which() == type_vec[i]);
          json_assert(elt.max.which() == elt.max.npos || elt.max.which() == type_vec[i]);
          for(const auto& range_elt : elt.values)
          {
            json_assert(range_elt.which() == range_elt.npos || range_elt.which() == type_vec[i]);
          }
        }
      }
      else
      {
        // We create default ranges
        p.ranges.resize(type_vec.size());
      }

      auto clipmode_it = obj.find(key::attribute<ClipModes>());
      if(clipmode_it != obj.end())
      {
        p.clipmodes = jsonToClipModeArray(clipmode_it->second);

        json_assert(p.clipmodes.size() == type_vec.size());
      }
      else
      {
        // We create default clip-modes
        p.clipmodes.resize(type_vec.size(), ClipMode::Both);
      }
    }

    map.insert(p);
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
