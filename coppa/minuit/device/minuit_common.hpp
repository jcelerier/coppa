#pragma once
#include <coppa/minuit/parameter.hpp>
#include <coppa/string_view.hpp>

namespace coppa
{
namespace ossia
{

enum class minuit_command : char
{ Request = '?', Answer = ':', Error = '!' };

enum class minuit_operation : char
{ Listen = 'l', Namespace = 'n', Get = 'g'};

enum class minuit_type : char
{ Application = 'A', Container = 'C', Data = 'D', None = 'n' };

enum class minuit_attributes
{ Value, Type, Service, Priority, RangeBounds, RangeClipMode, Description, RepetitionFilter };


auto to_minuit_type_text(const coppa::ossia::Parameter& parameter)
{
  // integer, decimal, string, generic, boolean, none, array.
  switch(parameter.variants.size())
  {
    case 0:
      return "none";
    case 1:
    {
      switch(which(parameter.variants[0]))
      {
        case Type::int_t:
          return "integer";
        case Type::float_t:
          return "decimal";
        case Type::bool_t:
          return "boolean";
        case Type::string_t:
          return "string";
        case Type::generic_t:
          return "generic";
        default:
          return "generic"; // TODO
      }
    }
    default:
      return "array";
  }
}


auto to_minuit_service_text(coppa::Access::Mode acc)
{
  switch(acc)
  {
    case Access::Mode::None:
      return ""; // TODO
    case Access::Mode::Both:
      return "parameter";
    case Access::Mode::Get:
      return "return";
    case Access::Mode::Set:
      return "message";
    default:
      throw;
  }
}

auto to_minuit_bounding_text(coppa::Bounding::Mode b)
{
  switch(b)
  {
    case Bounding::Mode::Free:
      return "free";
    case Bounding::Mode::Clip:
      return "clip";
    case Bounding::Mode::Wrap:
      return "wrap";
    case Bounding::Mode::Fold:
      return "fold";
    default:
      throw;
  }
}


minuit_attributes get_attribute(string_view str)
{
  // requires str.size() > 0
  switch(str[0])
  {
    case 'v': // value
      return minuit_attributes::Value;
    case 't': // type
      return minuit_attributes::Type;
    case 's': // service
      return minuit_attributes::Service;
    case 'p': // priority
      return minuit_attributes::Priority;
    case 'r':
    {
      if(str.size() >= 6)
      {
        switch(str[5])
        {
          case 'B': // rangeBounds
            return minuit_attributes::RangeBounds;
          case 'C': // rangeClipMode
            return minuit_attributes::RangeClipMode;
          case 'i': // repetitionsFilter
            return minuit_attributes::RepetitionFilter;
        }
      }
      // if not returning, throw
    }
    default:
      throw std::runtime_error("unhandled attribute");
  }
}

minuit_command get_command(char str)
{
  switch(str)
  {
    case '?':
    case ':':
    case '!':
      return static_cast<minuit_command>(str);
    default :
      throw std::runtime_error("unhandled command");
  }
}

minuit_type get_type(char str)
{
  switch(str)
  {
    case 'A':
    case 'C':
    case 'D':
    case 'n':
      return static_cast<minuit_type>(str);
    default :
      throw std::runtime_error("unhandled type");
  }
}

minuit_operation get_operation(char str)
{
  switch(str)
  {
    case 'l':
    case 'n':
    case 'g':
      return static_cast<minuit_operation>(str);
    default :
      throw std::runtime_error("unhandled operation");
  }
}

minuit_operation get_operation(string_view str)
{
  return get_operation(str[0]);
}

}
}
