#pragma once
#include <string>
#include <vector>
#include <utility>
#include <eggs/variant.hpp>

#define FORWARD_CTOR(ctor, obj) \
    template<typename... Args> \
    ctor(Args&&... args):\
        obj{std::forward<Args>(args)...} \
    { }

#define FORWARD_FUN(object, ret, fun) \
  template<typename... Args> \
  ret fun(Args&&... args) { \
    return object.fun(std::forward<Args>(args)...); \
  }

#define FORWARD_FUN_CONST(object, ret, fun) \
    template<typename... Args> \
    ret fun(Args&&... args) const { \
    return object.fun(std::forward<Args>(args)...); \
    }

#define FORWARD_LAMBDA(fun) [&] (auto&&... args) { return this->fun(std::forward<decltype(args)>(args)...); }

#define GETTER(obj) auto& obj() noexcept { return m_##obj; }
#define GETTER_CONST(obj) auto& obj() const noexcept { return m_##obj; }

#if defined(coppa_dynamic)
#define coppa_name(theName) static constexpr const char * name{ #theName };
#define coppa_parameter(theValue) \
  const auto& get() const { return theValue; } \
  template<typename T> void set(T&& t) { theValue = t; }
#else
#define coppa_name(theName)
#define coppa_parameter(theValue)
#endif


namespace coppa
{
// Definition of standard attributes
template<typename ValueType>
struct SimpleValue
{
    coppa_name(SimpleValue)
    ValueType value;
};

using Tag = std::string;
struct Tags
{
    coppa_name(Tags)
    std::vector<Tag> tags;
};

struct Alias
{
    coppa_name(Alias)
    std::string alias;
};

struct Description
{
    coppa_name(Description)
    std::string description;
};

struct Access
{
    enum class Mode { None = 0, Get = 1, Set = 2, Both = 3 };
    coppa_name(Access)
    Mode accessmode{};
};

struct Destination
{
    coppa_name(Destination)
    std::string destination;
};

enum class ClipMode { None, Low, High, Both };

// The binding class.
template<typename... Args>
class AttributeAggregate : public Args...
{
};
// Make an oscquery-specific Variant ?

struct Generic
{
    std::string buf;

    bool operator==(const Generic& other) const
    {
      return buf == other.buf;
    }
};

enum class Type { int_t, float_t, bool_t, string_t, generic_t };

// TODO boost::recursive_variant
// TODO map Type to the actual types somehow
using Variant = eggs::variant<int, float, bool, std::string, Generic>;

inline char getOSCType(const coppa::Variant& value)
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


}
