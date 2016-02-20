#pragma once
#include <coppa/oscquery/parameter.hpp>
#include <oscpack/osc/OscOutboundPacketStream.h>

inline oscpack::OutboundPacketStream& operator<<(
    oscpack::OutboundPacketStream& p,
    const coppa::oscquery::Variant& val)
{
  using namespace eggs::variants;
  using namespace coppa::oscquery;
  switch(which(val))
  {
    case Type::int_t:
      p << get<int>(val);
      break;
    case Type::float_t:
      p << get<float>(val);
      break;
    case Type::bool_t:
      p << get<bool>(val);
      break;
    case Type::string_t:
      p << get<std::string>(val).c_str();
      break;
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
    const coppa::vector<coppa::oscquery::Variant>& values)
{
  using namespace coppa;

  for(const auto& val : values)
  {
    p << val;
  }

  return p;
}
