#pragma once
#include <string>
#include <vector>
#include <utility>
#include <eggs/variant.hpp>

#define FORWARD_FUN(object, ret, fun) \
  template<typename... Args> \
  ret fun(Args&&... args) { \
    return object.fun(std::forward<Args>(args)...); \
  }

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

using Generic = std::vector<char>;

enum class Type { int_t, float_t, bool_t, string_t, generic_t };
using Variant = eggs::variant<int, float, bool, std::string, Generic>;

}
