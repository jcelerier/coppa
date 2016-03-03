#pragma once
#include <coppa/minuit/device/osc_device.hpp>
#include <coppa/minuit/parameter.hpp>
#include <coppa/minuit/device/message_handler.hpp>
#include <coppa/minuit/device/minuit_local_behaviour.hpp>
#include <coppa/minuit/device/minuit_remote_behaviour.hpp>
#include <coppa/map.hpp>

#include <coppa/protocol/osc/oscreceiver.hpp>
#include <coppa/protocol/osc/oscsender.hpp>
#include <coppa/protocol/osc/oscmessagegenerator.hpp>
#include <coppa/string_view.hpp>
namespace coppa
{
namespace ossia
{
class minuit_local_impl : public osc_local_device<
    coppa::locked_map<coppa::basic_map<ParameterMapType<coppa::ossia::Parameter>>>,
    coppa::osc::receiver,
    coppa::ossia::minuit_message_handler<minuit_local_behaviour>,
    coppa::osc::sender>
{
  public:
    minuit_local_impl(
        const std::string& name,
        map_type& map,
        unsigned int in_port,
        std::string out_ip,
        unsigned int out_port):
      osc_local_device{map, in_port, out_ip, out_port,
                       [&] (const auto& m, const auto& ip) {
      data_handler_t::on_messageReceived(*this, this->map(), m, ip);
    }},
      nameTable{name}
    {

    }

    minuit_name_table nameTable;
};


}
}
