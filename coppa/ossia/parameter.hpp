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

struct Value
{
    Variant value;
};

inline Type which(const Variant& var)
{
  return static_cast<Type>(var.which());
}

small_string getOSCType(const Variant& value);
small_string getOSCType(const Tuple& value);


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

inline small_string getOSCType(const Tuple& tuple)
{
  using namespace oscpack;
  small_string str;
  for(const auto& val : tuple.variants)
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
using Range = coppa::Range<Variant>;
using Parameter = AttributeAggregate<
  Value,
  Range,
  Destination,
  Description,
  Access,
  Bounding,
  RepetitionFilter>;
}
}
