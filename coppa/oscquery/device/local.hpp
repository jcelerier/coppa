#pragma once
#include <coppa/oscquery/device/queryparser.hpp>
#include <coppa/oscquery/device/remoteclient.hpp>
#include <coppa/oscquery/websockets.hpp>
#include <coppa/protocol/osc/oscreceiver.hpp>
#include <coppa/protocol/osc/oscsender.hpp>
#include <coppa/protocol/osc/oscmessagegenerator.hpp>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <coppa/oscquery/map.hpp>
#include <coppa/oscquery/json/writer.hpp>
#include <coppa/oscquery/device/queryanswerer.hpp>
namespace coppa
{
namespace osc
{
class message_handler : public coppa::osc::receiver
{
  public:
    template<typename Device>
    static void on_messageReceived(Device& dev, const ::oscpack::ReceivedMessage& m)
    {
      using namespace coppa;

      if(dev.map().has(m.AddressPattern()))
      {
        dev.update(
              m.AddressPattern(),
              [&] (auto& v) {
          int i = 0;
          for(auto it = m.ArgumentsBegin(); it != m.ArgumentsEnd(); ++it, ++i)
          {
            // Note : how to handle mismatch between received osc messages
            // and the structure of the tree ?

            // TODO assert correct number of args
            auto& elt = v.values[i];
            switch((coppa::Type)elt.which())
            {
              case Type::int_t:
                elt = it->AsInt32();
                break;
              case Type::float_t:
                elt = it->AsFloat();
                break;
              case Type::bool_t:
                elt = it->AsBool();
                break;
              case Type::string_t:
                elt = std::string(it->AsString());
                break;
                //  TODO case Type::generic_t: array.add(get<const char*>(val)); break;

              default:
                break;
            }
          }
        });
      }
    }
};

}
}
namespace coppa
{
namespace oscquery
{
template<typename QueryServer,
         typename QueryAnswerer,
         typename Writer,
         typename DataServer,
         typename DataHandler>
class LocalDevice
{
  public:
    using query_server = QueryServer;
    using query_answerer = QueryAnswerer;

    const auto& clients() const
    { return m_clients; }
    auto& clients()
    { return m_clients; }
    auto& server()
    { return m_server; }

    LocalDevice()
    {
      m_receiver.run();
    }

    template<typename T>
    void addHandler(const std::string& name, T&& fun)
    {
      m_handlers[name] = fun;
    }

    void removeHandler(const std::string& name)
    {
      m_handlers.erase(name);
    }

    void add(const Parameter& parameter)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.add(parameter);
    }

    void remove(const std::string& path)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.remove(path);
    }

    template<typename Arg>
    void update(const std::string& path, Arg&& val)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.update(path, std::forward<Arg>(val));

      if(m_handlers.find(path) != std::end(m_handlers))
      {
        m_handlers[path](m_map.get(path));
      }
    }

    template<typename Arg>
    void update_attributes(const std::string& path, Arg&& val)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.update_attributes(path, std::forward<Arg>(val));

      if(m_handlers.find(path) != std::end(m_handlers))
      {
        m_handlers[path](m_map.get(path));
      }
    }

    void rename(std::string oldPath, std::string newPath)
    { /* todo PATH_CHANGED */ }

    Parameter get(const std::string& address) const
    { return m_map.get(address); }

    auto& map()
    { return m_map; }
    const auto& map() const
    { return m_map; }

    void expose()
    { m_server.run(); }

    auto& receiver() { return m_receiver; }

  private:
    DataServer m_receiver{
      1234,
      [&] (const auto& m) { DataHandler::on_messageReceived(*this, m); }
    };

    mutable std::mutex m_map_mutex;
    LockedParameterMap<SimpleParameterMap<ParameterMap>> m_map;
    std::vector<RemoteClient<QueryServer>> m_clients;
    std::unordered_map<std::string, std::function<void(const Parameter&)>> m_handlers;
    QueryServer m_server
    {
      // Open handler
      [&] (typename QueryServer::connection_handler hdl)
      {
        // TODO lock this too
        m_clients.emplace_back(hdl);

        // Send the client a message with the OSC port
        m_server.sendMessage(hdl, Writer::deviceInfo(m_receiver.port()));
      },
      // Close handler
      [&] (typename QueryServer::connection_handler hdl)
      {
        auto it = std::find(begin(m_clients), end(m_clients), hdl);
        if(it != end(m_clients))
        {
          m_clients.erase(it);
        }
      },
      // Message handler
      [&] (typename QueryServer::connection_handler hdl, const std::string& message)
      {
        std::lock_guard<std::mutex> lock(m_map_mutex);
        return QueryParser::parse(
              message,
              QueryAnswerer::answer(*this, hdl));
      }
    };
};

// Automatically synchronizes its changes with the client.
// Note : put somewhere if the client wants to be synchronized
// Note : be careful with the locking too, here
template<typename QueryServer,
         typename QueryAnswerer,
         typename Writer,
         typename DataServer,
         typename DataHandler>
class SynchronizingLocalDevice
{
  private:
    LocalDevice<QueryServer, QueryAnswerer, Writer, DataServer, DataHandler> m_device;

  public:
    SynchronizingLocalDevice()
    {
    }

    template<typename... Args>
    void addHandler(Args&&... args)
    { m_device.addHandler(std::forward<Args>(args)...); }
    template<typename... Args>
    void removeHandler(Args&&... args)
    { m_device.removeHandler(std::forward<Args>(args)...); }

    auto& device() { return m_device; }

    void add(const Parameter& parameter)
    {
      m_device.add(parameter);

      auto message = Writer::path_added(
                       m_device.map().unsafeMap(),
                       parameter.destination);
      for(auto& client : m_device.clients())
      {
        m_device.server().sendMessage(client, message);
      }
    }

    void remove(const std::string& path)
    {
      m_device.remove(path);

      auto message = Writer::path_removed(path);
      for(auto& client : m_device.clients())
      {
        m_device.server().sendMessage(client, message);
      }
    }

    template<typename... Attributes>
    void update_attributes(const std::string& path, Attributes&&... val)
    {
      m_device.update_attributes(path, std::forward<Attributes>(val)...);

      auto message = Writer::attributes_changed(
                       path, std::forward<Attributes>(val)...);
      for(auto& client : m_device.clients())
      {
        m_device.server().sendMessage(client, message);
      }
    }

    void rename(std::string oldPath, std::string newPath)
    { /* todo PATH_CHANGED */ }

    Parameter get(const std::string& address) const
    { return m_device.get(address); }

    auto& map()
    { return m_device.map(); }
    const auto& map() const
    { return m_device.map(); }

    void expose()
    { m_device.expose(); }
};

}
}
