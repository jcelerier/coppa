#pragma once
#include <coppa/ossia/device/osc_device.hpp>
#include <coppa/ossia/parameter.hpp>
#include <coppa/ossia/device/minuit_remote_future_behaviour.hpp>
#include <coppa/ossia/device/message_handler.hpp>
#include <coppa/ossia/device/minuit_name_table.hpp>
#include <coppa/map.hpp>

#include <coppa/protocol/osc/oscreceiver.hpp>
#include <coppa/protocol/osc/oscsender.hpp>

namespace coppa
{
namespace ossia
{
class minuit_remote_impl_future : public osc_local_device<
    locked_map<basic_map<ParameterMapType<Parameter>>>,
    osc::receiver,
    minuit_message_handler<minuit_callback_behaviour_wrapper_t>,
    osc::sender>
{
  public:
    minuit_remote_impl_future(
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

    auto find(const std::string& address)
    {
        return map().find(address);
    }

    auto pull(const std::string& address)
    {
        auto act = nameTable.get_action(minuit_action::GetRequest);
        this->sender.send(act, string_view(address));

        m_getPromises.emplace_back(address);
        return m_getPromises.back().promise.get_future();
    }

    template<typename Values_T>
    auto set(const std::string& address, Values_T&& values)
    {
        this->template update<std::string>(address, [&] (auto& p) {
            static_cast<coppa::ossia::Value&>(p) = std::forward<Values_T>(values);
        });
    }

    template<typename Values_T>
    auto push(const std::string& address, Values_T&& values)
    {
        this->sender.send(address, values);
        this->set(address, std::forward<Values_T>(values));
    }

    void listen(const std::string& address, bool b)
    {
      auto act = nameTable.get_action(minuit_action::ListenRequest);
      if(b)
        this->sender.send(act, string_view(address), "enable");
      else
        this->sender.send(act, string_view(address), "disable");
    }

    auto refresh()
    {
        if(m_namespaceRequests.empty())
        {
            map().clear();
            auto act = nameTable.get_action(minuit_action::NamespaceRequest);
            this->sender.send(act, "/");

            m_nsPromise = std::promise<void>{};
            return m_nsPromise.get_future();
        }
        else
        {
            return m_nsPromise.get_future();
        }
    }

    auto refresh(string_view act, const std::string& root)
    {
        auto it = m_namespaceRequests.find(root);
        if(it == m_namespaceRequests.end())
        {
            m_namespaceRequests.insert(root);

            sender.send(act, string_view(root));
        }
    }

    minuit_name_table nameTable;

    std::list<get_promise> m_getPromises;
    std::promise<void> m_nsPromise;
    std::set<std::string, std::less<>> m_namespaceRequests;
};

}
}
