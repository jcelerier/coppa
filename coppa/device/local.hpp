#pragma once
#include <coppa/coppa.hpp>
#include <coppa/device/remoteclient.hpp>

#include <unordered_map>
#include <algorithm>
#include <mutex>
namespace coppa
{

// As an example, make devices that also have :
// no callbacks at all
// callbacks on structural changes
// callbacks that are only called when not updating locally.

/**
 * @brief The local_device class
 *
 * An example of generic device. This device has the following capabilities :
 * - answering to requests using QueryAnswerer (for instance paths requested via an http address=
 * - updating the tree both by the reception of messages that uses DataProtocolServer/Handler,
 *   and by local intervention
 * - ability to register per-address handlers that will inform the local software.
 */
template<typename Map,
         typename QueryServer,
         typename QueryParser,
         typename QueryAnswerer,
         typename Serializer,
         typename DataProtocolServer,
         typename DataProtocolHandler>
class local_device : public Map
{
  public:
    using map_type = Map;
    using query_server_type = QueryServer;
    using query_answerer_type = QueryAnswerer;
    using serializer_type = Serializer;

    GETTER(clients)
    GETTER_CONST(clients)

    GETTER(query_server)

    void expose()
    { m_query_server.run(); }

    auto& receiver() { return m_data_server; }

    local_device()
    {
      initDataServer(1234);
      initQueryServer();
    }

    local_device(int data_port)
    {
      initDataServer(data_port);
      initQueryServer();
    }

    local_device(
        QueryServer&& query_serv):
      m_query_server{std::move(query_serv)}
    {
      initDataServer(1234);
      initQueryServer();
    }

    local_device(
        QueryServer&& query_serv,
        DataProtocolServer&& data_serv):
      m_data_server{std::move(data_serv)},
      m_query_server{std::move(query_serv)}
    {
      m_data_server.run();
      initQueryServer();
    }

    // Handlers to be called when an address is modified.
    template<typename T>
    void addHandler(const std::string& name, T&& fun)
    {
      m_handlers[name] = fun;
    }

    void removeHandler(const std::string& name)
    {
      m_handlers.erase(name);
    }

    template<typename Arg>
    void update(const std::string& path, Arg&& val)
    {
      Map::update(path, std::forward<Arg>(val));

      if(m_handlers.find(path) != std::end(m_handlers))
      {
        m_handlers[path](Map::get(path));
      }
    }

    template<typename Arg>
    void update_attributes(const std::string& path, Arg&& val)
    {
      Map::update_attributes(path, std::forward<Arg>(val));

      if(m_handlers.find(path) != std::end(m_handlers))
      {
        m_handlers[path](Map::get(path));
      }
    }

    void rename(std::string oldPath, std::string newPath)
    { /* todo PATH_CHANGED */ }

  private:
    //// Data server behaviour ////
    DataProtocolServer m_data_server; // Note : why not directly a list of servers on a data-access layer???

    void initDataServer(int port)
    {
      m_data_server = DataProtocolServer(port,
                                         [&] (const auto& m)
      { DataProtocolHandler::on_messageReceived(*this, m); });

      m_data_server.run();
    }

    //// Query server behaviour ////
    QueryServer m_query_server;
    void initQueryServer()
    {
      m_query_server.setOpenHandler(FORWARD_LAMBDA(on_connectionOpen));
      m_query_server.setCloseHandler(FORWARD_LAMBDA(on_connectionClosed));
      m_query_server.setMessageHandler(FORWARD_LAMBDA(on_message));
    }

    // Handlers for the query server
    void on_connectionOpen(typename QueryServer::connection_handler hdl)
    {
      // TODO This should be locked by inside? Use a thread-safe vector ?
      m_clients.emplace_back(hdl);

      // Send the client a message with the OSC port
      m_query_server.sendMessage(hdl, Serializer::deviceInfo(m_data_server.port()));
    }

    void on_connectionClosed(typename QueryServer::connection_handler hdl)
    {
      auto it = std::find(begin(m_clients), end(m_clients), hdl);
      if(it != end(m_clients))
      {
        m_clients.erase(it);
      }
    }

    auto on_message(typename QueryServer::connection_handler hdl, const std::string& message)
    {
      return QueryParser::parse(
            message,
            QueryAnswerer::answer(*this, hdl));
    }

    std::vector<remote_client<QueryServer>> m_clients;
    std::unordered_map<std::string, std::function<void(const typename Map::value_type&)>> m_handlers;
};


/**
 * @brief The synchronizing_local_device class
 *
 * Extends a local_device :
 * Automatically synchronizes its changes with the client.
 * Note : put somewhere if the client wants to be synchronized
 * Note : be careful with the locking too, here
 */
template<typename... Args>
class synchronizing_local_device : private local_device<Args...>
{
    using parent_t = local_device<Args...>;
  public:
    using parent_t::parent_t;
    using parent_t::addHandler;
    using parent_t::removeHandler;
    using parent_t::expose;

    using parent_t::size;
    using parent_t::has;
    using parent_t::operator[];

    template<typename T>
    void add(const T& parameter)
    {
      parent_t::add(parameter);

      auto message = parent_t::serializer_type::path_added(
                       parent_t::unsafeMap(),
                       parameter.destination);
      for(auto& client : parent_t::clients())
      {
        parent_t::query_server().sendMessage(client, message);
      }
    }

    void remove(const std::string& path)
    {
      parent_t::remove(path);

      auto message = parent_t::serializer_type::path_removed(path);
      for(auto& client : parent_t::clients())
      {
        parent_t::query_server().sendMessage(client, message);
      }
    }

    template<typename... Attributes>
    void update_attributes(const std::string& path, Attributes&&... val)
    {
      parent_t::update_attributes(path, std::forward<Attributes>(val)...);

      auto message = parent_t::serializer_type::attributes_changed(
                       path, std::forward<Attributes>(val)...);
      for(auto& client : parent_t::clients())
      {
        parent_t::query_server().sendMessage(client, message);
      }
    }

    void rename(std::string oldPath, std::string newPath)
    { /* todo PATH_CHANGED */ }
};

}

