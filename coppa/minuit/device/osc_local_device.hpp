#pragma once
#include <coppa/minuit/device/osc_device.hpp>
#include <coppa/minuit/parameter.hpp>
#include <coppa/map.hpp>
#include <coppa/protocol/osc/oscsender.hpp>

namespace coppa
{
namespace ossia
{

class osc_local_impl : public osc_local_device<
    coppa::locked_map<coppa::basic_map<ParameterMapType<coppa::ossia::Parameter>>>,
    coppa::osc::receiver,
    coppa::ossia::osc_message_handler,
    coppa::osc::sender>
{
  public:
    osc_local_impl(
        map_type& map,
        unsigned int in_port,
        std::string out_ip,
        unsigned int out_port):
      osc_local_device{map, in_port, out_ip, out_port,
                       [&] (const auto& m, const auto& ip) {
      data_handler_t::on_messageReceived(*this, this->map(), m, ip);
    }}
    {

    }
};

}
}
