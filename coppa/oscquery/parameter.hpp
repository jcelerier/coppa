#pragma once
#include <coppa/coppa.hpp>
#include <coppa/map.hpp>

namespace coppa
{
namespace oscquery
{
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
      && lhs.clipmodes == rhs.clipmodes
      && lhs.description == rhs.description
      && lhs.tags == rhs.tags;
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

}
}
