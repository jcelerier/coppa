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
 */
class message_handler : public coppa::osc::receiver
{
  public:
    template<typename Device>
    static void on_messageReceived(Device& dev, const oscpack::ReceivedMessage& m)
    {
      using namespace coppa;

      if(dev.has(m.AddressPattern()))
      {
        dev.update(
              m.AddressPattern(),
              [&] (auto& v) {
          int i = 0;
          for(auto it = m.ArgumentsBegin(); it != m.ArgumentsEnd(); ++it, ++i)
          {
            // Note : how to handle mismatch between received osc messages
            // and the structure of the tree ?

            // TODO assert correct number of args
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
                //  TODO case Type::generic_t: array.add(get<const char*>(val)); break;

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
