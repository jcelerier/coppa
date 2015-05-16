#pragma once
#include <coppa/oscquery/device/queryparser.hpp>
#include <coppa/oscquery/device/remoteclient.hpp>
#include <coppa/oscquery/websockets.hpp>
#include <coppa/protocol/osc/oscreceiver.hpp>
#include <coppa/protocol/osc/oscsender.hpp>
#include <coppa/protocol/osc/oscmessagegenerator.hpp>
#include <unordered_set>
#include <map>
#include <coppa/oscquery/map.hpp>
#include <coppa/oscquery/json/writer.hpp>
#include <coppa/oscquery/device/queryanswerer.hpp>
namespace coppa
{
namespace oscquery
{
template<typename QueryServer,
         typename QueryAnswerer>
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
      // TODO Add osc message handlers to update the tree
      // when an osc message is received.
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

    template<typename Attribute>
    void update(const std::string& path, const Attribute& val)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.update(path, [=] (Parameter& p) { static_cast<Attribute&>(p) = val; });
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

  private:
    OscReceiver m_receiver{1234};
    mutable std::mutex m_map_mutex;
    LockedParameterMap<SimpleParameterMap<ParameterMap>> m_map;
    std::vector<RemoteClient<QueryServer>> m_clients;
    QueryServer m_server
    {
      // Open handler
      [&] (typename QueryServer::connection_handler hdl)
      {
        // TODO lock this too
        m_clients.emplace_back(hdl);

        // Send the client a message with the OSC port
        m_server.sendMessage(hdl, JSON::writer::deviceInfo(m_receiver.port()));
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
template<typename QueryServer, typename QueryAnswerer>
class SynchronizingLocalDevice
{
  private:
    LocalDevice<QueryServer, QueryAnswerer> m_device;

  public:
    SynchronizingLocalDevice()
    {
    }

    void add(const Parameter& parameter)
    {
      m_device.add(parameter);

      auto message = JSON::writer::path_added(m_device.map().unsafeMap(), parameter.destination);
      for(auto& client : m_device.clients())
      {
        m_device.server().sendMessage(client, message);
      }
    }

    void remove(const std::string& path)
    {
      m_device.remove(path);

      auto message = JSON::writer::path_removed(path);
      for(auto& client : m_device.clients())
      {
        m_device.server().sendMessage(client, message);
      }
    }


    template<typename Attribute, typename... Attributes>
    void update(const std::string& path, const Attribute& attr, Attributes&&... val)
    {
      m_device.update(path, attr);
      update(path, std::forward<Attributes>(val)...);
    }

    template<typename... Attributes>
    void update(const std::string& path, Attributes&&... val)
    {
      m_device.update(path, std::forward<Attributes>(val)...);

      auto message = JSON::writer::attributes_changed(
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
