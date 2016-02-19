#pragma once
#include <coppa/coppa.hpp>
#include <oscpack/osc/OscOutboundPacketStream.h>
#include <boost/container/small_vector.hpp>

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
      p << oscpack::BeginBundleImmediate << oscpack::BeginMessage( name.c_str() );
      subfunc(args...);

      return p;
    }

    template<typename Val_T>
    const oscpack::OutboundPacketStream& operator()(
        const std::string& name,
        const std::vector<Val_T>& values)
    {
      p.Clear();
      p << oscpack::BeginBundleImmediate
        << oscpack::BeginMessage( name.c_str() )
        << values
        << oscpack::EndMessage
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

    boost::container::small_vector<char, BufferSize> buffer;
    oscpack::OutboundPacketStream p{oscpack::OutboundPacketStream(buffer.data(), buffer.size())};
};
}
