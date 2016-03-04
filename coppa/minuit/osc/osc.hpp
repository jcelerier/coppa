#pragma once
#include <oscpack/osc/OscOutboundPacketStream.h>
#include <coppa/minuit/parameter.hpp>
#include <coppa/string_view.hpp>

namespace coppa
{
namespace ossia
{
inline oscpack::OutboundPacketStream& operator<<(
    oscpack::OutboundPacketStream& p,
    const coppa::ossia::Variant& val);


inline oscpack::OutboundPacketStream& operator<<(
    oscpack::OutboundPacketStream& p,
    const coppa::ossia::Values& values)
{
  using namespace coppa;

  for(const auto& val : values.variants)
  {
    p << val;
  }

  return p;
}

inline oscpack::OutboundPacketStream& operator<<(
    oscpack::OutboundPacketStream& p,
    const coppa::ossia::Tuple& values)
{
  using namespace coppa;

  p << oscpack::ArrayInitiator{};
  for(const auto& val : values.variants)
  {
    p << val;
  }
  p << oscpack::ArrayTerminator{};

  return p;
}

inline oscpack::OutboundPacketStream& operator<<(
    oscpack::OutboundPacketStream& p,
    const coppa::ossia::Variant& val)
{
  using namespace eggs::variants;
  using namespace coppa::ossia;
  // TODO visitor
  // See TTOscSocket::SendMessage;
  switch(which(val))
  {
    case Type::float_t:
      p << get<float>(val);
      break;
    case Type::int_t:
      p << get<int32_t>(val);
      break;
    case Type::impulse_t:
      // Seems like ossia isn't implemented like this : p << oscpack::InfinitumType{};
      break;
    case Type::bool_t:
      p << int32_t(get<bool>(val));
      break;
    case Type::string_t:
      p << string_view(get<std::string>(val));
      break;
    case Type::char_t:
      p << int32_t(get<char>(val));
      break;
    case Type::tuple_t:
    {
      p << get<Tuple>(val);
      break;
    }
    case Type::generic_t:
    {
      const auto& buf = get<coppa::Generic>(val);
      oscpack::Blob b(buf.buf.data(), buf.buf.size()); // todo : use Generic instead and convert to hex / base64
      p << b;
      break;
    }
    default:
      break;
  }

  return p;
}



inline oscpack::OutboundPacketStream& operator<<(
    oscpack::OutboundPacketStream& p,
    const coppa::ossia::Range& range)
{
  using namespace coppa;

  p << range.min << range.max;

  return p;
}

}
}
