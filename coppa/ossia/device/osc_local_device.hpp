#pragma once
#include <coppa/ossia/device/osc_device.hpp>
#include <coppa/ossia/parameter.hpp>
#include <coppa/map.hpp>
#include <coppa/protocol/osc/oscsender.hpp>
#include <coppa/protocol/osc/oscreceiver.hpp>
#include <coppa/ossia/device/message_handler.hpp>

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
        const std::string& name,
        map_type& map,
        unsigned int in_port,
        std::string out_ip,
        unsigned int out_port):
      osc_local_device{map, in_port, out_ip, out_port,
                       [&] (const auto& m, const auto& ip) {
      data_handler_t::on_messageReceived(*this, this->map(), m, ip);
    }},
      m_name{name}
    {

    }

    void set_name(const std::string& n)
    { m_name = n; }
    std::string get_name() const
    { return m_name; }


  private:
    std::string m_name;
};

}
}
