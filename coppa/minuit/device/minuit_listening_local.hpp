#pragma once
#include <coppa/minuit/parameter.hpp>
#include <coppa/device/local.hpp>
#include <coppa/minuit/device/message_handler.hpp>
#include <coppa/minuit/device/minuit_local_behaviour.hpp>
#include <coppa/map.hpp>

#include <coppa/protocol/osc/oscreceiver.hpp>
#include <coppa/protocol/osc/oscsender.hpp>
#include <coppa/protocol/osc/oscmessagegenerator.hpp>
#include <coppa/string_view.hpp>
#include <nano-signal-slot/nano_signal_slot.hpp>
#include <array>

namespace coppa
{
namespace ossia
{

struct listened_attribute
{
    listened_attribute() = default;
    listened_attribute(const std::string& other):
      path{other}
    {

    }

    listened_attribute(string_view other):
      path{other}
    {

    }

    std::string path;

    void enable(minuit_attribute a) const
    {
      attributes[static_cast<int>(a)] = true;
    }
    void disable(minuit_attribute a) const
    {
      attributes[static_cast<int>(a)] = false;
    }

    bool empty() const
    {
      return std::find(std::begin(attributes), std::end(attributes), true) == std::end(attributes);
    }

    mutable std::array<bool, 8> attributes;

    friend
    bool operator<(const std::string& other, const listened_attribute& attr)
    {
      return attr.path < other;
    }

    friend
    bool operator<(const listened_attribute& attr, const std::string& other)
    {
      return attr.path < other;
    }

    friend
    bool operator<(string_view other, const listened_attribute& attr)
    {
      return attr.path < other;
    }

    friend
    bool operator<(const listened_attribute& attr, string_view other)
    {
      return attr.path < other;
    }

    friend
    bool operator<(const listened_attribute& lhs, const listened_attribute& rhs)
    {
      return lhs.path < rhs.path;
    }
};

// The "local" behaviour only answers to requests.
template<minuit_command Req, minuit_operation Op>
struct minuit_listening_local_behaviour
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      return minuit_local_behaviour<Req,Op>{}(dev, map, mess);
    }
};

template<>
struct minuit_listening_local_behaviour<
    minuit_command::Request,
    minuit_operation::Listen>
{

    template<typename Device, typename Map>
    void handle_listening(
        Device& dev,
        Map& map,
        string_view address,
        minuit_attribute attr,
        bool enablement)
    {
      auto listen_it = dev.client.listened.find(address);
      if(listen_it != dev.client.listened.end())
      {
        if(enablement)
        {
          listen_it->enable(attr);
        }
        else
        {
          listen_it->disable(attr);
          if(listen_it->empty())
            dev.client.listened.erase(listen_it);
        }
      }
      else
      {
        if(enablement)
        {
          auto it = map.find(address);
          if(it != map.end())
          {
            auto res_it = dev.client.listened.emplace(address);
            res_it.first->enable(attr);
          }
        }
      }
    }

    template<typename Device, typename Map>
    auto operator()(
        Device& dev,
        Map& map,
        const oscpack::ReceivedMessage& mess)
    {
      // First argument will be "/WhereToListen:attribute"
      // B?listen /WhereToListen:attribute enable (turn on the listening)
      // B?listen /WhereToListen:attribute disable (turn off the listening)
      if(mess.ArgumentCount() != 2)
        return;

      auto arg_it = mess.ArgumentsBegin();
      string_view full_address{arg_it->AsString()};
      arg_it++;
      bool enablement = arg_it->AsString()[0] == 'e';

      auto idx = full_address.find_first_of(":");

      if(idx == std::string::npos)
      {
        handle_listening(dev, map, full_address, minuit_attribute::Value, enablement);
      }
      else
      {
        string_view address{full_address.data(), idx};

        // Note : bug if address == "foo:"
        auto attr = get_attribute(
                      string_view(
                        address.data() + idx + 1,
                        full_address.size() - idx - 1));

        handle_listening(dev, map, address, attr, enablement);
      }
    }
};


struct listener
{
    listener(const std::string& ip, int port):
      sender{ip, port}
    {

    }

    coppa::osc::sender sender;
    std::set<listened_attribute, std::less<>> listened;
};

// This one supports a single listener
class minuit_listening_local_device
{
  public:
    using map_type = coppa::locked_map<coppa::basic_map<ParameterMapType<coppa::ossia::Parameter>>>;
    using parent_t = minuit_listening_local_device;

    minuit_listening_local_device(
        map_type& map,
        unsigned int in_port,
        std::string out_ip,
        unsigned int out_port):
      m_map{map},
      m_server{in_port, [&] (const auto& m, const auto& ip)
      {
        coppa::ossia::minuit_message_handler<minuit_listening_local_behaviour>::on_messageReceived(*this, m_map, m, ip);
      }},
      client{out_ip, int(out_port)}
    {
      m_server.run();
    }

    std::string name() const
    { return "newDevice"; }

    auto& map() const
    { return m_map; }

    template<typename String, typename Arg>
    void update(param_t<String> path, Arg&& val)
    {
      if(m_map.update(path, std::forward<Arg>(val)))
      {
        auto res = m_map.get(path);
        on_value_changed.emit(res);

        auto it = client.listened.find(path);
        if(it != client.listened.end())
        {
          // A:listen /WhereToListen:attribute value (each time the attribute change if the listening is turned on)
          auto addr = name() + ":listen";
          auto cmd = path.to_string();
          cmd += ":";
          cmd += to_minuit_attribute_text(minuit_attribute::Value);

          client.sender.send(
                string_view(addr),
                string_view(cmd),
                static_cast<const Values&>(res));
        }

      }
    }

    Nano::Signal<void(const Parameter&)> on_value_changed;

    listener client;
    coppa::osc::sender& sender = client.sender;
  private:
    map_type& m_map;
    coppa::osc::receiver m_server;
};
}
}
