#pragma once
#include <coppa/coppa.hpp>
#include <oscpack/osc/OscOutboundPacketStream.h>
#include <array>
#include <boost/container/small_vector.hpp>
#include <coppa/string_view.hpp>

#include <iostream>
inline oscpack::OutboundPacketStream& operator<<(
    oscpack::OutboundPacketStream& p,
    const std::vector<coppa::string_view>& values)
{
  using namespace coppa;

  for(auto val : values)
  {
    p << val;
  }

  return p;
}

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
    const oscpack::OutboundPacketStream&  operator()(
        const std::string& name,
        const T&... args)
    {
      p.Clear();
      p << oscpack::BeginMessage( name.c_str() );
      subfunc(args...);
      p << oscpack::EndMessage;
      return p;
    }

    template<typename Val_T>
    const oscpack::OutboundPacketStream& operator()(
        const std::string& name,
        const std::vector<Val_T>& values)
    {
      p.Clear();
      p << oscpack::BeginMessage( name.c_str() )
        << values
        << oscpack::EndMessage;
      return p;
    }

    const oscpack::OutboundPacketStream& stream() const
    {
      return p;
    }

  private:
    void subfunc()
    {
    }

    template <typename Arg1, typename... Args>
    void subfunc(Arg1&& arg1, Args&&... args)
    {
      p << arg1;
      subfunc(args...);
    }

    std::array<char, BufferSize> buffer;
    oscpack::OutboundPacketStream p{buffer.data(), buffer.size()};
};
}
