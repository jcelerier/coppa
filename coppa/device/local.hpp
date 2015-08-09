#pragma once
#include <coppa/coppa.hpp>
#include <coppa/device/remoteclient.hpp>

#include <unordered_map>
#include <algorithm>
#include <mutex>
namespace coppa
{
template<typename Map,
         typename QueryServer,
         typename QueryParser,
         typename QueryAnswerer,
         typename Serializer,
         typename DataProtocolServer,
         typename DataProtocolHandler>
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

    LocalDevice(int data_port):
      m_receiver{data_port, [&] (const auto& m) { DataProtocolHandler::on_messageReceived(*this, m); }}
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

    template<typename T>
    void add(const T& parameter)
    {
      //std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.add(parameter);
    }

    void remove(const std::string& path)
    {
      //std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.remove(path);
    }

    template<typename Arg>
    void update(const std::string& path, Arg&& val)
    {
      //std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.update(path, std::forward<Arg>(val));

      if(m_handlers.find(path) != std::end(m_handlers))
      {
        m_handlers[path](m_map.get(path));
      }
    }

    template<typename Arg>
    void update_attributes(const std::string& path, Arg&& val)
    {
      //std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.update_attributes(path, std::forward<Arg>(val));

      if(m_handlers.find(path) != std::end(m_handlers))
      {
        m_handlers[path](m_map.get(path));
      }
    }

    void rename(std::string oldPath, std::string newPath)
    { /* todo PATH_CHANGED */ }

    auto get(const std::string& address) const
    { return m_map.get(address); }

    auto& map()
    { return m_map; }
    const auto& map() const
    { return m_map; }

    void expose()
    { m_server.run(); }

    auto& receiver() { return m_receiver; }

  private:
    DataProtocolServer m_receiver{
      1234,
      [&] (const auto& m) { DataProtocolHandler::on_messageReceived(*this, m); }
    };

    mutable std::mutex m_map_mutex;
    Map m_map;
    std::vector<RemoteClient<QueryServer>> m_clients;
    std::unordered_map<std::string, std::function<void(const typename Map::value_type&)>> m_handlers;
    QueryServer m_server
    {
      // Open handler
      [&] (typename QueryServer::connection_handler hdl)
      {
        // TODO lock this too
        m_clients.emplace_back(hdl);

        // Send the client a message with the OSC port
        m_server.sendMessage(hdl, Serializer::deviceInfo(m_receiver.port()));
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
        //std::lock_guard<std::mutex> lock(m_map_mutex);
        return QueryParser::parse(
              message,
              QueryAnswerer::answer(*this, hdl));
      }
    };
};

// Automatically synchronizes its changes with the client.
// Note : put somewhere if the client wants to be synchronized
// Note : be careful with the locking too, here
template<
    typename Map,
    typename QueryServer,
    typename QueryParser,
    typename QueryAnswerer,
    typename Serializer,
    typename DataProtocolServer,
    typename DataProtocolHandler>
class SynchronizingLocalDevice
{
  private:
    LocalDevice<
      Map,
      QueryServer,
      QueryParser,
      QueryAnswerer,
      Serializer,
      DataProtocolServer,
      DataProtocolHandler> m_device;

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

    template<typename T>
    void add(const T& parameter)
    {
      m_device.add(parameter);

      auto message = Serializer::path_added(
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

      auto message = Serializer::path_removed(path);
      for(auto& client : m_device.clients())
      {
        m_device.server().sendMessage(client, message);
      }
    }

    template<typename... Attributes>
    void update_attributes(const std::string& path, Attributes&&... val)
    {
      m_device.update_attributes(path, std::forward<Attributes>(val)...);

      auto message = Serializer::attributes_changed(
                       path, std::forward<Attributes>(val)...);
      for(auto& client : m_device.clients())
      {
        m_device.server().sendMessage(client, message);
      }
    }

    void rename(std::string oldPath, std::string newPath)
    { /* todo PATH_CHANGED */ }

    typename Map::value_type get(const std::string& address) const
    { return m_device.get(address); }

    auto& map()
    { return m_device.map(); }
    const auto& map() const
    { return m_device.map(); }

    void expose()
    { m_device.expose(); }
};

}

