#pragma once
#include <string>
#include <coppa/map.hpp>
#include <coppa/device/messagetype.hpp>

namespace coppa
{

template<typename QueryProtocolClient, typename Parser>
class RemoteQueryClient
{
    QueryProtocolClient m_client;
    std::string m_serverURI;

  public:
    template<typename ParseHandler>
    RemoteQueryClient(const std::string& uri, ParseHandler&& handler):
      m_client{[=] (typename QueryProtocolClient::connection_handler hdl, const std::string& message)
    {
      if(message.empty())
        return;
      handler(message);
    }},
      m_serverURI{m_client.connect(uri)}
    {
    }

    // Ask for an update of a part of the namespace
    void update(const std::string& root = "/")
    {
      // Should be part of the query protocol abstraction
      m_client.sendMessage(root);
    }

    // Ask for an update of a single attribute
    void updateAttribute(const std::string& address, const std::string& attribute)
    {
      m_client.sendMessage(address + "?" + attribute);
    }

    void listenAddress(const std::string& address, bool b)
    {
      m_client.sendMessage(address + "?listen=" + (b? "true" : "false"));
    }

    const std::string uri() const
    { return m_serverURI; }

};

template<typename Map>
class ConstantMap
{
  protected:
    LockedParameterMap<Map> m_map;

  public:
    bool has(const std::string& address) const
    { return m_map.has(address); }

    auto get(const std::string& address) const
    { return m_map.get(address); }

    Map map() const
    { return m_map; }

    LockedParameterMap<Map>& safeMap()
    { return m_map; }

    void replace(Map&& map)
    { m_map = std::move(map); }
};

template<typename Map, typename DataProtocolSender>
class SettableMap : public ConstantMap<Map>
{
  private:
    DataProtocolSender m_sender;

  public:
    void connect(const std::string& uri, int port)
    { m_sender = DataProtocolSender{uri, port}; }

    template<typename... Args>
    void set(const std::string& address, Args&&... args)
    {
      auto param = ConstantMap<Map>::get(address);
      if(param.accessmode == Access::Mode::Set
      || param.accessmode == Access::Mode::Both)
      {
        m_sender.send(address, std::forward<Args>(args)...);
      }
    }
};

// Cases : fully static (midi), non-queryable (pure osc), queryable (minuit, oscquery)
template<
    typename MapType,
    typename Parser,
    typename QueryProtocolClient,
    typename RemoteMapBase>
class QueryRemoteDevice : public RemoteMapBase, public QueryProtocolClient
{
    void on_queryServerMessage(const std::string& message)
    try
    {
      switch(Parser::messageType(message))
      {
        case MessageType::Device:
          // DeviceHandler
          RemoteMapBase::connect(QueryProtocolClient::uri(), Parser::getPort(message));
          break;

        case MessageType::PathAdded:
          // PathAdded handler
          break;

        case MessageType::PathRemoved:
          // TODO differentiate between removing a parameter,
          // and removing a whole part of the tree

          break;

        case MessageType::PathChanged:
          // Pass the map for modificationr
          Parser::parsePathChanged(RemoteMapBase::safeMap(), message);
          break;

        default:
          RemoteMapBase::replace(Parser::template parseNamespace<MapType>(message));
          break;
      }
    }
    catch(std::runtime_error& e)
    {
      std::cerr << "Error while parsing: " << e.what() << "  ==>  " << message;
    }

  public:
    QueryRemoteDevice(const std::string& uri):
      QueryProtocolClient{uri, [&] (const std::string& mess) { on_queryServerMessage(mess); }}
    {

    }

};

}
