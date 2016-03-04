#pragma once
#include <nano-signal-slot/nano_signal_slot.hpp>
#include <coppa/string_view.hpp>
#include <coppa/minuit/parameter.hpp>

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



// Remote OSC device :
// Can just send data to the outside
// Consists in a map set by the user or loaded, and a sender

// Updating OSC device
// Can be set from t
// Local Minuit device, connected to a remote device (fixed)
// Local Minuit device, that will reply to every client (no registering)
// Local Minuit device, that will reply to every client (registering & garbage collection)
// Updating Minuit device

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
      sender{out_ip, int(out_port)},
      server{in_port, h},
      m_map{map}
    {
      server.run();
    }

    auto& map() const
    { return m_map; }

    template<typename String, typename Arg>
    void update(param_t<String> path, Arg&& val)
    {
      auto it = m_map.update(path, std::forward<Arg>(val));
      if(it != m_map.end())
        on_value_changed.emit(m_map.get(path));
    }

    Nano::Signal<void(const Parameter&)> on_value_changed;

    DataProtocolSender sender;
    DataProtocolServer server;
  private:
    Map& m_map;
};

}
}
