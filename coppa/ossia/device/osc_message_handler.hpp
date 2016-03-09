#pragma once
#include <coppa/protocol/osc/oscreceiver.hpp>
#include <coppa/ossia/parameter.hpp>
#include <coppa/ossia/device/osc_common.hpp>
#include <coppa/ossia/osc/osc.hpp>
#include <coppa/string_view.hpp>
namespace coppa
{
namespace ossia
{

struct convert_osc_handler
{
    template<typename Device>
    void operator()(
        Device& dev,
        Value current_parameter,
        string_view address,
        const oscpack::ReceivedMessage& m)
    {
      using namespace coppa;
      using coppa::ossia::Parameter;
      using eggs::variants::get;

        // TODO not used ?
      Value res = read_value(
             m.ArgumentsBegin(),
             m.ArgumentsEnd(),
             current_parameter);

      dev.template update<string_view>(
            address,
            [&] (auto& v) {
        v.value = current_parameter.value;
      });
    }
};

class osc_message_handler : public coppa::osc::receiver
{
  public:
    template<typename Device, typename Map>
    static void on_messageReceived(
        Device& dev,
        Map& map,
        const oscpack::ReceivedMessage& m,
        const oscpack::IpEndpointName& ip)
    {
      string_view address{m.AddressPattern()};
      Value current_parameter;

      {
        auto node_it = map.find(address);
        if(node_it == map.end())
          return;

        current_parameter = *node_it;
      }

      convert_osc_handler{}(dev, std::move(current_parameter), address, m);
    }
};
}
}
