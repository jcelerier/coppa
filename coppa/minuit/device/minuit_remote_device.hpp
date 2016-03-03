#pragma once
#include <coppa/minuit/device/osc_device.hpp>
#include <coppa/minuit/device/minuit_remote_behaviour.hpp>
#include <coppa/minuit/parameter.hpp>
#include <coppa/minuit/device/message_handler.hpp>
#include <coppa/minuit/device/minuit_name_table.hpp>
#include <coppa/map.hpp>

#include <coppa/protocol/osc/oscreceiver.hpp>
#include <coppa/protocol/osc/oscsender.hpp>

namespace coppa
{
namespace ossia
{
class minuit_remote_impl : public osc_local_device<
    locked_map<basic_map<ParameterMapType<Parameter>>>,
    osc::receiver,
    minuit_message_handler<minuit_remote_behaviour>,
    osc::sender>
{
  public:

    minuit_remote_impl(
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

    void set_name(const std::string& n)
    { nameTable.set_device_name(n); }
    std::string get_name() const
    { return nameTable.get_device_name().to_string(); }

    auto refresh(string_view act, const std::string& root)
    {
        sender.send(act, string_view(root));
    }

    minuit_name_table nameTable;
};
}
}
