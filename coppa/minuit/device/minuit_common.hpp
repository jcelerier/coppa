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

enum class minuit_action : int {
  NamespaceRequest,
  NamespaceReply,
  NamespaceError,
  GetRequest,
  GetReply,
  GetError,
  ListenRequest,
  ListenReply,
  ListenError
};

enum class minuit_type : char
{ Application = 'A', Container = 'C', Data = 'D', None = 'n' };

enum class minuit_attribute
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

auto from_minuit_type_text(string_view str)
{
  Values v;
  // integer, decimal, string, generic, boolean, none, array.

  switch(str[0])
  {
    case 'i': // integer
      v.variants.push_back(int32_t{});
    case 'd': // decimal
      v.variants.push_back(float{});
    case 's': // string
      v.variants.push_back(std::string{});
    case 'b': // boolean
      v.variants.push_back(bool{});
    case 'g': // generic
      v.variants.push_back(Generic{});
    case 'n': // none
      break;
    case 'a': // array
      break;
    default:
      throw;
  }

  return v; // none
}


auto to_minuit_service_text(coppa::Access::Mode acc)
{
  switch(acc)
  {
    case Access::Mode::None:
      return "none"; // TODO
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

Access from_minuit_service_text(string_view str)
{
  switch(str[0])
  {
    case 'n':
      return Access{Access::Mode::None};
    case 'p':
      return Access{Access::Mode::Both};
    case 'r':
      return Access{Access::Mode::Get};
    case 'm':
      return Access{Access::Mode::Set};
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


Bounding from_minuit_bounding_text(string_view str)
{
  switch(str[2]) // only unique character
  {
    case 'e': // frEe
      return Bounding{Bounding::Mode::Free};
    case 'i': // clIp
      return Bounding{Bounding::Mode::Clip};
    case 'a': // wrAp
      return Bounding{Bounding::Mode::Wrap};
    case 'l': // foLd
      return Bounding{Bounding::Mode::Fold};
    default:
      throw;
  }
}


auto to_minuit_attribute_text(minuit_attribute str)
{
  switch(str) // only unique character
  {
    case minuit_attribute::Value:
      return "value";
    case minuit_attribute::Service:
      return "service";
    case minuit_attribute::Type:
      return "type";
    case minuit_attribute::Priority:
      return "priority";
    case minuit_attribute::RangeBounds:
      return "rangeBounds";
    case minuit_attribute::RangeClipMode:
      return "clipMode";
    case minuit_attribute::Description:
      return "description";
    case minuit_attribute::RepetitionFilter:
      return "repetitionFilter";
    default:
      throw;
  }
}

minuit_attribute get_attribute(string_view str)
{
  // requires str.size() > 0
  switch(str[0])
  {
    case 'v': // value
      return minuit_attribute::Value;
    case 't': // type
      return minuit_attribute::Type;
    case 's': // service
      return minuit_attribute::Service;
    case 'p': // priority
      return minuit_attribute::Priority;
    case 'r':
    {
      if(str.size() >= 6)
      {
        switch(str[5])
        {
          case 'B': // rangeBounds
            return minuit_attribute::RangeBounds;
          case 'C': // rangeClipMode
            return minuit_attribute::RangeClipMode;
          case 'i': // repetitionsFilter
            return minuit_attribute::RepetitionFilter;
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
