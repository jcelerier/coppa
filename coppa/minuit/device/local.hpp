#pragma once
#include <coppa/minuit/parameter.hpp>
#include <coppa/device/local.hpp>
#include <coppa/minuit/device/message_handler.hpp>
#include <coppa/minuit/device/minuit_local_behaviour.hpp>
#include <coppa/minuit/device/minuit_remote_behaviour.hpp>
#include <coppa/map.hpp>

#include <coppa/protocol/osc/oscreceiver.hpp>
#include <coppa/protocol/osc/oscsender.hpp>
#include <coppa/protocol/osc/oscmessagegenerator.hpp>
#include <coppa/string_view.hpp>
#include <nano-signal-slot/nano_signal_slot.hpp>
namespace coppa
{
namespace ossia
{
// Servers to do :
// Local OSC device :
// Can just be set from the outside.

// callbacks : try with callback on each member,
// and global "value changed" callback


// Shmem callbacks ?
// IPC ?
template<typename Map,
         typename DataProtocolServer,
         typename DataProtocolHandler,
         typename DataProtocolSender
         >
class osc_local_device
{
  public:
    using map_type = Map;
    using parent_t = osc_local_device<Map, DataProtocolServer, DataProtocolHandler, DataProtocolSender>;
    using data_handler_t = DataProtocolHandler;

    template<typename Handler>
    osc_local_device(
        Map& map,
        unsigned int in_port,
        std::string out_ip,
        unsigned int out_port,
        Handler h):
      m_map{map},
      m_data_server{in_port, h},
      sender{out_ip, int(out_port)}
    {
      m_data_server.run();
    }

    std::string name() const
    { return "newDevice"; }

    auto& map() const
    { return m_map; }

    template<typename String, typename Arg>
    void update(param_t<String> path, Arg&& val)
    {
      if(m_map.update(path, std::forward<Arg>(val)))
        on_value_changed.emit(m_map.get(path));
    }

    Nano::Signal<void(const Parameter&)> on_value_changed;

    DataProtocolSender sender;
  private:
    Map& m_map;
    DataProtocolServer m_data_server;
};

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

    minuit_name_table nameTable;
};
// Remote OSC device :
// Can just send data to the outside
// Consists in a map set by the user or loaded, and a sender

// Updating OSC device
// Can be set from t
// Local Minuit device, connected to a remote device (fixed)
// Local Minuit device, that will reply to every client (no registering)
// Local Minuit device, that will reply to every client (registering & garbage collection)
// Updating Minuit device

}
}
