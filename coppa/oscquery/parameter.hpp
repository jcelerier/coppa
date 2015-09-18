#pragma once
#include <coppa/coppa.hpp>
#include <coppa/map.hpp>

namespace coppa
{
namespace oscquery
{
using coppa::Access;
using coppa::Alias;
using coppa::Description;
using coppa::Destination;
using coppa::Generic;
using coppa::Tags;

struct Range
{
    Variant min;
    Variant max;
    std::vector<Variant> values;
    bool operator==(const Range& other) const
    {
      return other.min == min && other.max == max && other.values == values;
    }
};

struct Values
{
    coppa_name(Values)
    std::vector<Variant> values;
};

struct Ranges
{
    coppa_name(Ranges)
    std::vector<oscquery::Range> ranges;
};

struct ClipModes
{
    coppa_name(ClipModes)
    std::vector<ClipMode> clipmodes;
};

// TODO try one based on something like QVariantMap
using Parameter = AttributeAggregate<
                    Destination,
                    Values,
                    Ranges,
                    Access,
                    ClipModes,
                    Description,
                    Tags>;

inline bool operator==(const Parameter& lhs, const Parameter& rhs)
{
  return lhs.destination == rhs.destination
      && lhs.values == rhs.values
      && lhs.ranges == rhs.ranges
      && lhs.accessmode == rhs.accessmode
      && lhs.clipmodes == rhs.clipmodes
      && lhs.description == rhs.description
      && lhs.tags == rhs.tags;
}

inline bool operator!=(const Parameter& lhs, const Parameter& rhs)
{
  return !(lhs == rhs);
}

// What about putting this in a class ?
inline void addValue(Parameter& parameter,
                     const Variant& var,
                     const oscquery::Range& range = {{}, {}, {}},
                     const ClipMode& clipmode = ClipMode::None)
{
  parameter.values.push_back(var);
  parameter.ranges.push_back(range);
  parameter.clipmodes.push_back(clipmode);
}

template<typename T>
void addDefaultValue(Parameter& parameter)
{
  parameter.values.push_back(T{});

  int n = parameter.values.size();
  parameter.ranges.resize(n);
  parameter.clipmodes.resize(n);
}

// TODOHow to factorize this ??
inline void addDefaultValue(Parameter& parameter, std::size_t value_type_id)
{
  switch(value_type_id)
  {
    case 0:
      addDefaultValue<int>(parameter);
      break;
    case 1:
      addDefaultValue<float>(parameter);
      break;
    case 3:
      addDefaultValue<std::string>(parameter);
      break;
    default:
      throw;
  }
}

}
}
