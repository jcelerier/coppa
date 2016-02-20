#pragma once
#include <coppa/coppa.hpp>
#include <coppa/oscquery/parameter.hpp>
#include <coppa/protocol/osc/oscreceiver.hpp>
namespace coppa
{
namespace osc
{
/**
 * @brief The message_handler class
 *
 * Handles messages received via OSC in a OSCQuery device.
 *
 * TODO use it to make a client that is able to receive updates via OSC
 * Note : this requires a server able to send updates
 * via OSC too.
 *
 * We need to extend remote_client with an osc sender,
 * and remote_device with an osc receiver.
 *
 * When connecting via WS a client should send its
 * local port to the server.
 *
 */
class message_handler : public coppa::osc::receiver
{
  public:
    template<typename Device>
    static void on_messageReceived(
        Device& dev,
        const oscpack::ReceivedMessage& m)
    {
      using namespace coppa;

      oscquery::Parameter v;
      // Little dance for thread-safe access to the current value
      {
        auto& map = dev.map();
        auto&& l = map.acquire_read_lock();
        auto node_it = map.find(m.AddressPattern());
        if(node_it == map.end())
          return;

        v = *node_it;
      }

      // First check the compatibility
      if(m.ArgumentCount() != v.values.size())
        return;

      int i = 0;
      for(auto it = m.ArgumentsBegin(); it != m.ArgumentsEnd(); ++it, ++i)
      {
        auto tag = it->TypeTag();
        if(tag != oscquery::getOSCType(v.values[i]))
          return;
      }

      // If everything is okay, we can update the device.
      dev.update(
            m.AddressPattern(),
            [&] (auto& v) {
        int i = 0;

        for(auto it = m.ArgumentsBegin(); it != m.ArgumentsEnd(); ++it, ++i)
        {
          // Note : how to handle mismatch between received osc messages
          // and the structure of the tree ?
          auto& elt = v.values[i];
          switch(oscquery::which(elt))
          {
            case oscquery::Type::int_t:
              elt = it->AsInt32();
              break;
            case oscquery::Type::float_t:
              elt = it->AsFloat();
              break;
            case oscquery::Type::bool_t:
              elt = it->AsBool();
              break;
            case oscquery::Type::string_t:
              elt = std::string(it->AsString());
              break;
            case oscquery::Type::generic_t:
            {
              int n = 0;
              const char* data{};
              it->AsBlob(reinterpret_cast<const void*&>(data), n);
              elt = coppa::Generic{std::string(data, n)};
              break;
            }
            default:
              break;
          }
        }
      });
    }
};

}
}
