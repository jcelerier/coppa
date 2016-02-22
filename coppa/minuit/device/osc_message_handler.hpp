#pragma once
#include <coppa/protocol/osc/oscreceiver.hpp>
#include <coppa/minuit/parameter.hpp>
#include <coppa/minuit/device/osc_common.hpp>
#include <coppa/minuit/osc/osc.hpp>
#include <coppa/string_view.hpp>
namespace coppa
{
namespace ossia
{
class osc_message_handler : public coppa::osc::receiver
{
  public:
    template<typename Device, typename Map>
    static void handleOSCMessage(
        Device& dev,
        Map& map,
        string_view address,
        const oscpack::ReceivedMessage& m)
    {
      using namespace coppa;
      using coppa::ossia::Parameter;
      using eggs::variants::get;

      Values current_parameter;
      // The lock is already acquired in the parent
      {
        auto node_it = map.find(address);
        if(node_it == map.end())
          return;

        current_parameter = *node_it;
      }

      // First check the compatibility
      if(getOSCType(current_parameter) != m.TypeTags())
        return;

      // Then write the arguments

      values_reader<
          strict_error_handler,
          conversion_mode::Prechecked>{}(
             m.ArgumentsBegin(),
             m.ArgumentsEnd(),
             current_parameter);

      dev.template update<string_view>(
            address,
            [&] (auto& v) {
        v.variants = current_parameter.variants;
      });
    }

    template<typename Device, typename Map>
    static void on_messageReceived(
        Device& dev,
        Map& map,
        const oscpack::ReceivedMessage& m,
        const oscpack::IpEndpointName& ip)
    {
      handleOSCMessage(dev, map, m.AddressPattern(), m);
    }
};
}
}
