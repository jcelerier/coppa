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

    bool connected() const
    { return m_client.connected(); }

    // Ask for an update of a part of the namespace
    void update(const std::string& root = "/")
    { m_client.sendMessage(root); }

    // Ask for an update of a single attribute
    void updateAttribute(const std::string& address, const std::string& attribute)
    { m_client.sendMessage(address + "?" + attribute); }

    void listenAddress(const std::string& address, bool b)
    { m_client.sendMessage(address + "?listen=" + (b? "true" : "false")); }

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

    template<typename Map_T>
    void replace(Map_T&& map)
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
    typename BaseMapType,
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
          RemoteMapBase::connect(QueryProtocolClient::uri(), Parser::getPort(message));
          if(onConnect) onConnect();
          break;

        case MessageType::PathAdded:
          Parser::template parsePathAdded<BaseMapType>(RemoteMapBase::safeMap(), message);
          if(onUpdate) onUpdate();
          break;

        case MessageType::PathRemoved:
          Parser::parsePathRemoved(RemoteMapBase::safeMap(), message);
          if(onUpdate) onUpdate();
          break;

        case MessageType::PathChanged:
          Parser::parsePathChanged(RemoteMapBase::safeMap(), message);
          if(onUpdate) onUpdate();
          break;

        case MessageType::AttributeChanged:
          Parser::parseAttributeChanged(RemoteMapBase::safeMap(), message);
          if(onUpdate) onUpdate();
          break;

        default:
          RemoteMapBase::replace(Parser::template parseNamespace<BaseMapType>(message));
          if(onUpdate) onUpdate();
          break;
      }
    }
    catch(std::exception& e)
    {
      std::cerr << "Error while parsing: " << e.what() << "  ==>  " << message;
    }

  public:
    QueryRemoteDevice(const std::string& uri):
      QueryProtocolClient{uri, [&] (const std::string& mess) { on_queryServerMessage(mess); }}
    {

    }

    std::function<void()> onConnect;
    std::function<void()> onUpdate;

};

}
