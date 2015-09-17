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
      m_client{[=] (typename QueryProtocolClient::connection_handler hdl, const std::string& message)
    {
      if(message.empty())
        return;
      handler(message);
    }},
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
    void queryConnect()
    { m_client.connect(m_serverURI); }

    void queryConnectAsync()
    {
      if(m_asyncServerThread.joinable())
        m_asyncServerThread.join();
      m_asyncServerThread = std::thread([&]{ m_client.connect(m_serverURI); });
    }

    bool queryConnected() const
    { return m_client.connected(); }

    // Ask for an update of a part of the namespace
    void queryNamespace(const std::string& root = "/")
    { m_client.sendMessage(root); }

    // Ask for an update of a single attribute
    void queryAttribute(const std::string& address, const std::string& attribute)
    { m_client.sendMessage(address + "?" + attribute); }

    void listenAddress(const std::string& address, bool b)
    { m_client.sendMessage(address + "?listen=" + (b? "true" : "false")); }

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
    typename RemoteMapBase>
class remote_query_device : public RemoteMapBase, public QueryProtocolClient
{
    void on_queryServerMessage(const std::string& message)
    try
    {
        // TODO the map is parsed twice. We should forward the json_map obtained here
        // and have the parser functions take json_map's.
      auto mt = Parser::messageType(message);

      if(mt == MessageType::Device)
      {
        RemoteMapBase::connect(QueryProtocolClient::uri(), Parser::getPort(message));
        if(onConnect) onConnect();
      }
      else
      {
        switch(mt)
        {
          case MessageType::Namespace:
            RemoteMapBase::replace(Parser::template parseNamespace<BaseMapType>(message));
            break;


          case MessageType::PathAdded:
            Parser::template path_added<BaseMapType>(RemoteMapBase::safeMap(), message);
            break;

          case MessageType::PathChanged:
            Parser::path_changed(RemoteMapBase::safeMap(), message);
            break;

          case MessageType::PathRemoved:
            Parser::path_removed(RemoteMapBase::safeMap(), message);
            break;

          case MessageType::AttributesChanged:
            Parser::attributes_changed(RemoteMapBase::safeMap(), message);
            break;


          case MessageType::PathsAdded:
            Parser::template paths_added<BaseMapType>(RemoteMapBase::safeMap(), message);
            break;

          case MessageType::PathsChanged:
            Parser::paths_changed(RemoteMapBase::safeMap(), message);
            break;

          case MessageType::PathsRemoved:
            Parser::paths_removed(RemoteMapBase::safeMap(), message);
            break;

          case MessageType::AttributesChangedArray:
            Parser::attributes_changed_array(RemoteMapBase::safeMap(), message);
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

  public:
    remote_query_device(const std::string& uri):
      QueryProtocolClient{uri, [&] (const std::string& mess) { on_queryServerMessage(mess); }}
    {

    }

    std::function<void()> onConnect;
    std::function<void()> onUpdate;
};

}
