#pragma once
#include <coppa/oscquery/parameter.hpp>
#include <coppa/oscquery/json/keys.hpp>
#include <coppa/exceptions/BadRequest.hpp>

#include <jeayeson/jeayeson.hpp>
#include <boost/tokenizer.hpp>
#include <boost/bimap.hpp>
#include <boost/assign.hpp>
namespace coppa
{
namespace oscquery
{
namespace json
{
namespace detail
{
inline void addValueToJsonArray(
    json_array& array,
    const Variant& val)
{
  using namespace eggs::variants;
  switch(val.which())
  {
    case 0: array.push_back(get<int>(val)); break;
    case 1: array.push_back(get<float>(val)); break;
      // TODO case 2: array.push_back(get<bool>(val)); break;
    case 3: array.push_back(get<std::string>(val)); break;
      // TODO base64 case 4: array.push_back(get<coppa::Generic>(val)); break;
  }
}

inline json_array getJsonValueArray(
    const Values& values)
{
  json_array value_arr;

  for(const auto& value : values.values)
  {
    addValueToJsonArray(value_arr, value);
  }

  return value_arr;
}

inline json_array getJsonClipModeArray(
    const ClipModes& clipmodes)
{
  static const boost::bimap<std::string, ClipMode> clipmodeMap =
      boost::assign::list_of<boost::bimap<std::string, ClipMode>::relation>
      ("None", ClipMode::None)
      ("Low",  ClipMode::Low)
      ("High", ClipMode::High)
      ("Both", ClipMode::Both);

  json_array clip_arr;
  for(const auto& clipmode : clipmodes.clipmodes)
  {
    clip_arr.push_back(clipmodeMap.right.at(clipmode));
  }

  return clip_arr;
}

inline json_array getJsonRangeArray(
    const Ranges& ranges)
{
  json_array range_arr;
  for(const auto& range : ranges.ranges)
  {
    json_array range_subarray;
    if(!range.min)
    { range_subarray.push_back(json_value::null_t{}); }
    else
    { addValueToJsonArray(range_subarray, range.min); }

    if(!range.max)
    { range_subarray.push_back(json_value::null_t{}); }
    else
    { addValueToJsonArray(range_subarray, range.max); }

    if(range.values.empty())
    { range_subarray.push_back(json_value::null_t{}); }
    else
    {
      json_array range_values_array;
      for(auto& elt : range.values)
      {
        addValueToJsonArray(range_values_array, elt);
      }
      range_subarray.push_back(range_values_array);
    }

    range_arr.push_back(range_subarray);
  }

  return range_arr;
}

inline json_array getJsonTags(
    const Tags& tags)
{
  json_array arr;
  for(const auto& tag : tags.tags)
  {
    arr.push_back(tag);
  }

  return arr;
}


inline std::string getJsonTypeString(
    const Parameter& parameter)
{
  std::string str_type;
  for(const auto& value : parameter.values)
  {
    switch(value.which())
    {
      case 0: str_type += "i"; break;
      case 1: str_type += "f"; break;
        // TODO case 2: str_type += "B"; break; -> no bool
      case 3: str_type += "s"; break;
      case 4: str_type += "b"; break;
    }
  }

  return str_type;
}

inline auto attributeToJson(const Destination& val) { return val.destination; }
inline auto attributeToJson(const Values& val) { return getJsonValueArray(val); }
inline auto attributeToJson(const Ranges& val) { return getJsonRangeArray(val); }
inline auto attributeToJson(const ClipModes& val) { return getJsonClipModeArray(val); }
inline auto attributeToJson(const Access& val) { return static_cast<int>(val.accessmode); }
inline auto attributeToJson(const Description& val) { return val.description; }
inline auto attributeToJson(const Tags& val) { return getJsonTags(val); }

inline auto attributeToJsonValue(
    const Parameter& parameter,
    const std::string& method
    )
{
  /**/ if(method == key::attribute<Values>())
  { return json_value(attributeToJson(static_cast<const Values&>(parameter))); }
  else if(method == key::attribute<Ranges>())
  { return json_value(attributeToJson(static_cast<const Ranges&>(parameter))); }
  else if(method == key::attribute<ClipModes>())
  { return json_value(attributeToJson(static_cast<const ClipModes&>(parameter))); }
  else if(method == key::attribute<Access>())
  { return json_value(attributeToJson(static_cast<const Access&>(parameter))); }
  else if(method == key::type())
  { return json_value(getJsonTypeString(parameter)); }
  else if(method == key::attribute<Description>())
  { return json_value(attributeToJson(static_cast<const Description&>(parameter))); }
  else if(method == key::attribute<Tags>())
  { return json_value(attributeToJson(static_cast<const Tags&>(parameter))); }
  else if(method == key::full_path())
  { return json_value(attributeToJson(static_cast<const Destination&>(parameter))); }
  else {
    throw BadRequestException{"Attribute not found"};
  }
}

inline void parameterToJson(
    const Parameter& parameter,
    json_map& obj)
{
  using namespace std;
  using namespace boost;
  using namespace eggs::variants;

  // These attributes are always here
  obj.set(key::full_path(), parameter.destination);
  obj.set(key::attribute<Access>(), static_cast<int>(parameter.accessmode));

  // Potentially empty attributes :
  // Description
  if(!parameter.description.empty())
  {
    obj.set(key::attribute<Description>(), parameter.description);
  }

  // Tags
  if(!parameter.tags.empty())
  {
    obj.set(key::attribute<Tags>(), getJsonTags(parameter));
  }

  // Handling of the types / values
  if(!parameter.values.empty())
  {
    obj.set(key::type(), getJsonTypeString(parameter));
    obj.set(key::attribute<Values>(), getJsonValueArray(parameter));
    obj.set(key::attribute<Ranges>(), getJsonRangeArray(parameter));
    obj.set(key::attribute<ClipModes>(), getJsonClipModeArray(parameter));
  }
}

// A ParameterMap can be JSON'd
template<typename Map>
json_map mapToJson(
    const Map& theMap,
    const std::string& root)
{
  using namespace std;
  using namespace boost;
  using namespace eggs::variants;
  // Root node
  json_map localroot;

  // Create a tree with the parameters
  for(const auto& parameter : filter(theMap, root))
  {
    // Truncate the given root from the parameters
    auto trunked_dest = parameter.destination;
    if(root != "/")
      trunked_dest.erase(0, root.length());

    char_separator<char> sep("/");
    tokenizer<char_separator<char>> tokens(trunked_dest, sep);

    // Create the required parts of the tree and navigate to the corresponding node
    auto current_map = &localroot;
    for(const auto& token : tokens)
    {
      // Note : see this in relation the osc method part of the spec
      if(!current_map->has(key::contents()))
      { current_map->set(key::contents(), json_map{}); }

      current_map = &current_map->get_for_path<json_map>(key::contents());

      if(!current_map->has(token))
      { current_map->set(token, json_map{}); }

      current_map = &current_map->get_for_path<json_map>(token);
    }

    parameterToJson(parameter, *current_map);
  }

  return localroot;
}
} // End of namespace detail
}
}
}
