#pragma once
#include <coppa/coppa.hpp>
#include <coppa/map.hpp>
#include <boost/container/small_vector.hpp>

namespace coppa
{
namespace oscquery
{

using coppa::Access;
using coppa::Alias;
using coppa::Description;
using coppa::Destination;
using coppa::Tags;
using coppa::Generic;

enum class Type { int_t, float_t, bool_t, string_t, generic_t };

// TODO boost::recursive_variant
// TODO map Type to the actual types somehow
using Variant = eggs::variant<int, float, bool, std::string, Generic>;
using Range = coppa::Range<Variant>;

inline Type which(const Variant& var)
{
  return static_cast<Type>(var.which());
}

inline char getOSCType(const Variant& value)
{
  switch(value.which())
  {
    case 0: return 'i';
    case 1: return 'f';
    case 2: return eggs::variants::get<bool>(value) ? 'T' : 'F';
    case 3: return 's';
    case 4: return 'b';
    default: return 'N';
  }
}

template<typename T>
using vector = boost::container::small_vector<T, 6>;

struct Values
{
    coppa_name(Values)
    vector<Variant> values;
};

struct Ranges
{
    coppa_name(Ranges)
    vector<oscquery::Range> ranges;
};

struct ClipModes
{
    coppa_name(ClipModes)
    vector<ClipMode> clipmodes;
};

// TODO try one based on something like QVariantMap
// TODO another possibility would be to use boost::optional
// for access / clipmode
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
      && lhs.access == rhs.access
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

using ParameterMap = ParameterMapType<Parameter>;

}
}
