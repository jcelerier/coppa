#pragma once
#include <coppa/coppa.hpp>
#include <boost/container/static_vector.hpp>
#include <oscpack/osc/SmallString.h>
namespace coppa
{
namespace ossia
{
using oscpack::small_string;
using coppa::Access;
using coppa::Bounding;
using coppa::Alias;
using coppa::Description;
using coppa::Destination;
using coppa::Tags;
using coppa::Generic;

enum class Type { none_t, impulse_t, bool_t, int_t, float_t, char_t, string_t, tuple_t, generic_t };
struct None {};
struct Impulse {};
struct Tuple;
using Variant = eggs::variant<None, Impulse, bool, int32_t, float, char, std::string, Tuple, Generic>;

struct Tuple
{
    Tuple() = default;
    Tuple(std::initializer_list<Variant> lst):
      variants(lst)
    {

    }

    std::vector<Variant> variants;
};

/**
 * @brief The Values struct
 * Note : while the same in definition, Tuple and Values
 * differ in semantics.
 *
 * Values is for storing the values in an OSC message :
 * /a/b/c iii 1 2 3
 * -> Values{1, 2, 3}
 *
 * Tuples is the oscpack array type :
 * /a/b/c i[[ii][ff]] 1 [[2, 3], [4.5, 5.2]]
 * -> Values{1, Tuple{Tuple{2, 3}, Tuple{4.5, 5.2}}}
 *
 * Todo experiment for specific cases when n =< big_n
 */
struct Values
{
    vector<Variant> variants;
};

inline Type which(const Variant& var)
{
  return static_cast<Type>(var.which());
}

small_string getOSCType(const Variant& value);
small_string getOSCType(const Tuple& value);
small_string getOSCType(const Values& value);


inline small_string getOSCType(const Variant& value)
{
  using eggs::variants::get;
  using namespace oscpack;

  switch(which(value))
  {
    case Type::impulse_t: return small_string(1, INFINITUM_TYPE_TAG);
    case Type::int_t: return small_string(1, INT32_TYPE_TAG);
    case Type::float_t: return small_string(1, FLOAT_TYPE_TAG);
    case Type::bool_t: return small_string(1, get<bool>(value) ? TRUE_TYPE_TAG : FALSE_TYPE_TAG);
    case Type::char_t: return small_string(1, CHAR_TYPE_TAG);
    case Type::string_t: return small_string(1, STRING_TYPE_TAG);
    case Type::tuple_t: return getOSCType(get<Tuple>(value));
    case Type::generic_t: return  small_string(1, BLOB_TYPE_TAG);
    default: return small_string(1, NIL_TYPE_TAG);
  }
}

inline bool operator!=(const small_string& string, const char* ptr)
{
  return strcmp(string.data(), ptr);
}

small_string getOSCType(const Tuple& tuple)
{
  using namespace oscpack;
  small_string str(1, ARRAY_BEGIN_TYPE_TAG);
  for(const auto& val : tuple.variants)
  {
    str.append(getOSCType(val));
  }
  str.push_back(ARRAY_END_TYPE_TAG);
  return str;
}

small_string getOSCType(const Values& values)
{
  using namespace oscpack;
  small_string str;
  for(const auto& val : values.variants)
  {
    str.append(getOSCType(val));
  }
  return str;
}


inline bool operator==(const None& lhs, const None& rhs)
{ return true; }
inline bool operator==(const Impulse& lhs, const Impulse& rhs)
{ return true; }
inline bool operator==(const Tuple& lhs, const Tuple& rhs)
{ return lhs.variants == rhs.variants; }

struct RepetitionFilter
{
    coppa_name(RepetitionFilter)
    bool repetitionFilter{};
};

template<typename ValueType> using Enum = std::vector<ValueType>;

using Parameter = AttributeAggregate<
  Values,
  Destination,
  Description,
  Access,
  Bounding,
  RepetitionFilter>;
/*
template<typename ValueType>
struct Interval
{
    enum class Type { OpenOpen, OpenClosed, ClosedOpen, ClosedClosed } type;
    std::pair<ValueType, ValueType> range;
};

template<typename ValueType>
using DomainType = std::vector<eggs::variant<Interval<ValueType>, Enum<ValueType>>>;

template<typename DomainValueType,
         typename InfComparator,
         typename EqComparator>
class Bounds
{
  public:
    coppa_name(Bounds)

    DomainType<DomainValueType> domain;
    BoundingMode lower_bound = BoundingMode::Free;
    BoundingMode upper_bound = BoundingMode::Free;

    template<typename ValueType>
    bool valueIsInBounds(ValueType val)
    {
      using namespace eggs::variants;
      for(auto&& subdomain : domain)
      {
        if(const auto& interval = get<Interval<DomainValueType>>(subdomain))
        {
          if(InfComparator::operatorInf(interval.range.first, val))
          {
          }
        }
        else
        {
          const auto& enumeration = get<Enum<DomainValueType>>(subdomain);
          for(auto enum_val : enumeration)
          {
            if(EqComparator::operatorEq(val, enum_val))
            {
            }
          }
        }
      }
    }
};

template<class ValueType>
class StandardComparator
{
    static bool operatorInf(const ValueType& lhs, const ValueType& rhs)
    { return lhs < rhs; }
    static bool operatorEq(const ValueType& lhs, const ValueType& rhs)
    { return lhs == rhs; }
};

using Parameter = ParameterAdapter<SimpleValue<Variant>,
Tags,
Alias,
RepetitionFilter,
Bounds<Variant,
StandardComparator<Variant>,
StandardComparator<Variant>>>;
*/
}
}
