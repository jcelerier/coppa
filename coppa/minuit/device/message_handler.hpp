#pragma once
#include <coppa/minuit/parameter.hpp>
#include <coppa/map.hpp>
#include <coppa/minuit/device/minuit_common.hpp>
#include <coppa/minuit/device/osc_message_handler.hpp>
#include <coppa/protocol/osc/oscreceiver.hpp>
#include <oscpack/osc/OscTypes.h>
#include <oscpack/osc/OscOutboundPacketStream.h>
#include <oscpack/osc/OscDebug.h>

namespace coppa
{
namespace ossia
{

// Namespace request :
// app?namespace addr

// Namespace answer :
// app:namespace addr minuit_type nodes={ n1 n2 n3 } attributes={ foo bar baz }
template<
    template<
      minuit_command,
      minuit_operation>
    typename Handler>
class minuit_message_handler :
    public coppa::osc::receiver
{
  public:
    template<typename Device, typename Map>
    static void handleMinuitMessage(
        Device& dev,
        Map& map,
        string_view address,
        const oscpack::ReceivedMessage& m)
    {
      // Look for either ':' or '?'
      auto idx = address.find_first_of(":?!");

      if(idx != std::string::npos)
      {
        //string_view application(address.data(), idx);
        //string_view request(address.data() + idx + 1, address.size() - application.size() - 1);

        //std::cerr << application << " " << request << std::endl;
        auto req = get_command(address[idx]);
        auto op = get_operation(*(address.data() + idx + 1));
        switch(req)
        {
          case minuit_command::Answer: // Receiving an answer
          {
            switch(op)
            {
              case minuit_operation::Listen:
                Handler<minuit_command::Answer, minuit_operation::Listen>{}(dev, map, m);
                break;
              case minuit_operation::Get:
                Handler<minuit_command::Answer, minuit_operation::Get>{}(dev, map, m);
                break;
              case minuit_operation::Namespace:
                Handler<minuit_command::Answer, minuit_operation::Namespace>{}(dev, map, m);
                break;
              default:
                break;
            }
            break;
          }
          case minuit_command::Request: // Receiving a request
          {
            switch(op)
            {
              case minuit_operation::Listen:
                Handler<minuit_command::Request, minuit_operation::Listen>{}(dev, map, m);
                break;
              case minuit_operation::Get:
                Handler<minuit_command::Request, minuit_operation::Get>{}(dev, map, m);
                break;
              case minuit_operation::Namespace:
                Handler<minuit_command::Request, minuit_operation::Namespace>{}(dev, map, m);
                break;
              default:
                break;
            }
            break;
          }
          case minuit_command::Error: // Receiving an error
          {
            switch(op)
            {
              case minuit_operation::Listen:
                Handler<minuit_command::Error, minuit_operation::Listen>{}(dev, map, m);
                break;
              case minuit_operation::Get:
                Handler<minuit_command::Error, minuit_operation::Get>{}(dev, map, m);
                break;
              case minuit_operation::Namespace:
                Handler<minuit_command::Error, minuit_operation::Namespace>{}(dev, map, m);
                break;
              default:
                break;
            }
            break;
          }
          default:
            break;
        }
      }

      // For minuit address, the request is the address pattern,
      // and the arugments may contain the address of type 's', and the OSC stuff.
    }

    template<typename Device, typename Map>
    static void on_messageReceived(
        Device& dev,
        Map& map,
        const oscpack::ReceivedMessage& m,
        const oscpack::IpEndpointName& ip)
    {
      auto l = map.acquire_write_lock();
      string_view address{m.AddressPattern()};
      oscpack::debug(std::cerr, m);
      // We have to check if it's a plain osc address, or a Minuit request address.

      if(address.size() > 0 && address[0] == '/')
      {
        osc_message_handler::handleOSCMessage(dev, map.get_data_map(), address, m);
      }
      else
      {
        handleMinuitMessage(dev, map.get_data_map(), address, m);
      }
    }
};
}
}
