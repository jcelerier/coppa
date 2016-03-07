#pragma once
#include <nano-signal-slot/nano_signal_slot.hpp>
#include <coppa/string_view.hpp>
#include <coppa/ossia/parameter.hpp>

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
      m_map.update(path, std::forward<Arg>(val));
    }

    std::string get_remote_ip() const
    { return sender.ip(); }
    void set_remote_ip(const std::string& ip)
    { sender = DataProtocolSender(ip, sender.port()); }

    int get_remote_input_port() const
    { return sender.port(); }
    void set_remote_input_port(int p)
    { sender = DataProtocolSender(sender.ip(), p); }

    int get_local_input_port() const
    { return server.port(); }
    void set_local_input_port(int p)
    { server.setPort(p); }

    auto find(const std::string& address)
    {
        return map().find(address);
    }

    template<typename Values_T>
    auto push(const std::string& address, Values_T&& values)
    {
        this->sender.send(address, values);
        this->set(address, std::forward<Values_T>(values));
    }

    template<typename Values_T>
    auto set(const std::string& address, Values_T&& values)
    {
        this->template update<std::string>(address, [&] (auto& p) {
            static_cast<coppa::ossia::Value&>(p) = std::forward<Values_T>(values);
        });
    }

    auto set_access(const std::string& address, coppa::ossia::Access::Mode am)
    {
        this->template update<std::string>(address, [&] (auto& p) {
            p.access = am;
        });
    }

    auto set_bounding(const std::string& address, coppa::ossia::Bounding::Mode bm)
    {
        this->template update<std::string>(address, [&] (auto& p) {
            p.bounding = bm;
        });
    }

    auto set_repetition(const std::string& address, bool rf)
    {
        this->template update<std::string>(address, [&] (auto& p) {
            p.repetitionFilter = rf;
        });
    }

    auto set_range(const std::string& address, coppa::ossia::Range&& r)
    {
        m_map.update_attributes(address, std::move(r));
    }

    DataProtocolSender sender;
    DataProtocolServer server;
  private:
    Map& m_map;
};

}
}
