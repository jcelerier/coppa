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
    Mode access{};
};

struct Bounding
{
    enum class Mode { Free = 0, Clip = 1, Wrap = 2, Fold = 3 };
    coppa_name(Bounding)
    Mode bounding{};
};

struct Destination
{
    coppa_name(Destination)
    std::string destination;
};

template<typename Var_T>
struct Range
{
    Var_T min;
    Var_T max;
    std::vector<Var_T> values;
    bool operator==(const Range& other) const
    {
      return other.min == min && other.max == max && other.values == values;
    }
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



}
