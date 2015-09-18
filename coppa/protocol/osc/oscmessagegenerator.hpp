#pragma once
#include <coppa/coppa.hpp>
#include <oscpack/osc/OscOutboundPacketStream.h>
#include <array>

namespace oscpack
{

template<int BufferSize = 1024>
class MessageGenerator
{
  public:
    MessageGenerator() = default;

    template <typename... T>
    MessageGenerator(const std::string& name, const T&... args)
    {
      operator()(name, args...);
    }

    template <typename... T>
    const oscpack::OutboundPacketStream&  operator()(const std::string& name, const T&... args)
    {
      p.Clear();
      p << oscpack::BeginBundleImmediate << oscpack::BeginMessage( name.c_str() );
      subfunc(args...);

      return p;
    }

    const oscpack::OutboundPacketStream&  operator()(const std::string& name,
                                                 const std::vector<coppa::Variant>& values)
    {
      using namespace eggs::variants;
      using namespace coppa;
      p.Clear();
      p << oscpack::BeginBundleImmediate
        << oscpack::BeginMessage( name.c_str() );
      for(const auto& val : values)
      {
        switch((coppa::Type)val.which())
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
      }
      p << oscpack::EndMessage
        << oscpack::EndBundle;

      return p;
    }

    const oscpack::OutboundPacketStream& stream() const
    {
      return p;
    }

  private:
    void subfunc()
    {
      p << oscpack::EndMessage << oscpack::EndBundle;
    }

    template <typename Arg1, typename... Args>
    void subfunc(Arg1&& arg1, Args&&... args)
    {
      p << arg1;
      subfunc(args...);
    }

    std::array<char, BufferSize> buffer;
    oscpack::OutboundPacketStream p{oscpack::OutboundPacketStream(buffer.data(), buffer.size())};
};
}
