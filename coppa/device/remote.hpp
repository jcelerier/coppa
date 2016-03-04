#pragma once
#include <string>
#include <thread>
#include <coppa/map.hpp>
#include <coppa/device/messagetype.hpp>

namespace coppa
{

/**
 * @brief The remote_query_client class
 *
 * A simple class that allows to connect to a query server
 * and send common queries on it
 */
template<
    typename QueryProtocolClient,
    typename Parser>
class remote_query_client
{
    QueryProtocolClient m_client;
    std::string m_serverURI;

    std::thread m_asyncServerThread;

  public:
    template<typename ParseHandler>
    remote_query_client(const std::string& uri, ParseHandler&& handler):
      m_client{
        [=] (
          typename QueryProtocolClient::connection_handler hdl,
          const std::string& message)
        {
          if(message.empty())
            return;
          handler(message);
        }
    },
      m_serverURI{uri}
    {
    }

    ~remote_query_client()
    {
      if(m_client.connected())
        m_client.stop();
      if(m_asyncServerThread.joinable())
        m_asyncServerThread.join();
    }

    // Is blocking
    void query_connect()
    { m_client.connect(m_serverURI); }

    void query_connect_async()
    {
      if(m_asyncServerThread.joinable())
        m_asyncServerThread.join();
      m_asyncServerThread = std::thread([&]{ m_client.connect(m_serverURI); });
    }

    bool query_is_connected() const
    { return m_client.connected(); }

    void query_close()
    { m_client.close(); }

    // Ask for an update of a part of the namespace
    void query_request_namespace(const std::string& root = "/")
    { m_client.send_message(root); }

    // Ask for an update of a single attribute
    void query_request_attribute(const std::string& address, const std::string& attribute)
    { m_client.send_message(address + "?" + attribute); }

    void query_listen_address(const std::string& address, bool b)
    { m_client.send_message(address + "?listen=" + (b? "true" : "false")); }

    const std::string uri() const
    { return m_serverURI; }

};


/**
 * @brief The remote_query_device class
 *
 * A device that answers to query servers. For instance it will mirror the
 * namespace data that it receives.
 *
 * It has simple user-settable callbacks for connection and message events.
 *
 * TODO make a more complete device with per-addresss callbacks.
 */
template<
    typename BaseMapType,
    typename Parser,
    typename QueryProtocolClient,
    typename RemoteMapSetter>
class remote_query_device :
    public QueryProtocolClient
{
  public:
    remote_query_device(RemoteMapSetter& map, const std::string& uri):
      QueryProtocolClient{uri, [&] (const std::string& mess) { on_query_server_message(mess); }},
      m_setter{map}
    {

    }

    auto& map()
    { return m_setter.map(); }

    auto& map() const
    { return m_setter.map(); }

    auto& setter() const
    { return m_setter; }

    std::function<void()> onConnect;
    std::function<void()> onUpdate;

  protected:
    RemoteMapSetter& m_setter;

  private:
    void on_query_server_message(const std::string& message)
    try
    {
      auto data = Parser::parse(message);
      auto mt = Parser::messageType(data);

      if(mt == MessageType::Device)
      {
        m_setter.connect(QueryProtocolClient::uri(), Parser::getPort(data));
        if(onConnect) onConnect();
      }
      else
      {
        if(mt == MessageType::Namespace)
        {
          // It has its own lock
          map() = Parser::template parseNamespace<BaseMapType>(data);
          if(onUpdate) onUpdate();
          return;
        }

        auto lock = map().acquire_write_lock();
        switch(mt)
        {
          case MessageType::PathAdded:
            Parser::template path_added<BaseMapType>(map().get_data_map(), data);
            break;

          case MessageType::PathChanged:
            Parser::path_changed(map().get_data_map(), data);
            break;

          case MessageType::PathRemoved:
            Parser::path_removed(map().get_data_map(), data);
            break;

          case MessageType::AttributesChanged:
            Parser::attributes_changed(map().get_data_map(), data);
            break;


          case MessageType::PathsAdded:
            Parser::template paths_added<BaseMapType>(map().get_data_map(), data);
            break;

          case MessageType::PathsChanged:
            Parser::paths_changed(map().get_data_map(), data);
            break;

          case MessageType::PathsRemoved:
            Parser::paths_removed(map().get_data_map(), data);
            break;

          case MessageType::AttributesChangedArray:
            Parser::attributes_changed_array(map().get_data_map(), data);
            break;

          case MessageType::Device:
          default:
            break;
        }

        if(onUpdate) onUpdate();
      }
    }
    catch(std::exception& e)
    {
      std::cerr << "Error while parsing: " << e.what() << "  ==>  " << message;
    }
};

}
