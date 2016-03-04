#pragma once
#include <coppa/minuit/device/osc_device.hpp>
#include <coppa/minuit/parameter.hpp>
#include <coppa/map.hpp>
#include <coppa/protocol/osc/oscsender.hpp>
#include <coppa/protocol/osc/oscreceiver.hpp>
#include <coppa/minuit/device/message_handler.hpp>

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

    auto find(const std::string& address)
    {
        return map().find(address);
    }

    template<typename Values_T>
    auto set(const std::string& address, Values_T&& values)
    {
        this->template update<std::string>(address, [&] (auto& p) {
            static_cast<coppa::ossia::Values&>(p) = std::forward<Values_T>(values);
        });
    }

    template<typename Values_T>
    auto push(const std::string& address, Values_T&& values)
    {
        this->sender.send(address, values);
        this->set(address, std::forward<Values_T>(values));
    }

    void set_name(const std::string& n)
    { m_name = n; }
    std::string get_name() const
    { return m_name; }

    std::string get_remote_ip() const
    { return sender.ip(); }
    void set_remote_ip(const std::string& ip)
    { sender = coppa::osc::sender(ip, sender.port()); }

    int get_remote_input_port() const
    { return sender.port(); }
    void set_remote_input_port(int p)
    { sender = coppa::osc::sender(sender.ip(), p); }

    int get_local_input_port() const
    { return server.port(); }
    void set_local_input_port(int p)
    { server.setPort(p); }

  private:
    std::string m_name;
};

}
}
