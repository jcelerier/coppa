#pragma once
#include <coppa/coppa.hpp>
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
 */
class message_handler : public coppa::osc::receiver
{
  public:
    template<typename Device>
    static void on_messageReceived(Device& dev, const oscpack::ReceivedMessage& m)
    {
      using namespace coppa;

      // TODO here we do two look-ups, we should lock, do just one (e.g. find()) and unlock.
      if(dev.has(m.AddressPattern()))
      {
        // First check the compatibility
        auto v = dev.get(m.AddressPattern());
        if(m.ArgumentCount() != v.values.size())
          return;

        int i = 0;
        for(auto it = m.ArgumentsBegin(); it != m.ArgumentsEnd(); ++it, ++i)
        {
          auto tag = it->TypeTag();
          if(tag != getOSCType(v.values[i]))
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
            switch((coppa::Type)elt.which())
            {
              case Type::int_t:
                elt = it->AsInt32();
                break;
              case Type::float_t:
                elt = it->AsFloat();
                break;
              case Type::bool_t:
                elt = it->AsBool();
                break;
              case Type::string_t:
                elt = std::string(it->AsString());
                break;
              case Type::generic_t:
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
    }
};

}
}
