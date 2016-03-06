#pragma once
#include <coppa/protocol/osc/oscreceiver.hpp>
#include <coppa/minuit/parameter.hpp>
#include <coppa/minuit/device/osc_common.hpp>
#include <coppa/minuit/osc/osc.hpp>
#include <coppa/string_view.hpp>
namespace coppa
{
namespace minuit
{
/**
 * @brief The strict_osc_handler struct
 *
 * This will be fast but will check that the
 * received OSC message is exactly of the same
 * type that the local type.
 */
struct strict_osc_handler
{
    template<typename Device>
    void operator()(
        Device& dev,
        Values current_parameter,
        string_view address,
        const oscpack::ReceivedMessage& m)
    {
      using namespace coppa;
      using coppa::minuit::Parameter;
      using eggs::variants::get;

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
        v.variants = std::move(current_parameter.variants);
      });
    }
};


/**
 * @brief The lax_osc_handler struct
 *
 * This will be fast, but will replace all
 * the current values and types by the ones received.
 */
struct lax_osc_handler
{
    template<typename Device>
    void operator()(
        Device& dev,
        Values current_parameter,
        string_view address,
        const oscpack::ReceivedMessage& m)
    {
      using namespace coppa;
      using coppa::minuit::Parameter;
      using eggs::variants::get;

      values_reader<
          lax_error_handler,
          conversion_mode::Replace>{}(
             m.ArgumentsBegin(),
             m.ArgumentsEnd(),
             current_parameter);

      dev.template update<string_view>(
            address,
            [&] (auto& v) {
        v.variants = current_parameter.variants;
      });
    }
};

struct convert_osc_handler
{
    template<typename Device>
    void operator()(
        Device& dev,
        Values current_parameter,
        string_view address,
        const oscpack::ReceivedMessage& m)
    {
      using namespace coppa;
      using coppa::minuit::Parameter;
      using eggs::variants::get;

      values_reader<
          lax_error_handler,
          conversion_mode::Convert>{}(
             m.ArgumentsBegin(),
             m.ArgumentsEnd(),
             current_parameter);

      dev.template update<string_view>(
            address,
            [&] (auto& v) {
        v.variants = current_parameter.variants;
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
      Values current_parameter;

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
