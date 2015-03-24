#pragma once
#include <coppa/oscquery/websockets.hpp>
#include <coppa/osc/oscreceiver.hpp>
#include <coppa/osc/oscsender.hpp>
#include <coppa/osc/oscmessagegenerator.hpp>
#include <unordered_set>
#include <coppa/oscquery/json/parser.hpp>

namespace coppa
{
namespace oscquery
{

template<typename QueryProtocolClient, typename Parser>
class RemoteQueryClient
{
    QueryProtocolClient m_client;

    std::string m_serverURI;

  public:
    template<typename ParseHandler>
    RemoteQueryClient(const std::string& uri, ParseHandler&& handler):
      m_client{[&] (typename QueryProtocolClient::connection_handler hdl, const std::string& message)
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

};

// Cases : fully static (midi), non-queryable (pure osc), queryable (minuit, oscquery)
template<
    typename MapType,
    typename Parser,
    typename DataProtocolSender,
    typename QueryProtocolClient>
class RemoteDevice : public QueryProtocolClient
{
    void on_queryServerMessage(const std::string& message)
    try
    {
      switch(Parser::messageType(message))
      {
        case MessageType::Device:
          // DeviceHandler
          //m_sender = OscSender{m_serverURI, JSONParser::getPort(message)};
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
          Parser::parsePathChanged(m_map, message);
          break;

        default:
          m_map = Parser::template parseNamespace<ParameterMap>(message);
          break;
      }
    }
    catch(BadRequestException& e)
    {
      std::cerr << "Error while parsing: " << e.what() << "  ==>  " << message;
    }

  public:
    RemoteDevice(const std::string& uri):
      QueryProtocolClient{uri, [&] (const std::string& mess) { on_queryServerMessage(mess); }}
    {

    }

    bool has(const std::string& address) const
    {
      return m_map.has(address);
    }

    // Get the local value
    Parameter get(const std::string& address) const
    {
      return m_map.get(address);
    }

    ParameterMap map() const
    {
      return m_map;
    }

    // Network operations
    void set(const std::string& address, const Values& val)
    {
      // Update local and send a message via OSC
      // Parameter must be settable
      auto param = get(address);
      if(param.accessmode == Access::Mode::Set
         || param.accessmode == Access::Mode::Both)
      {
        m_sender.send(osc::MessageGenerator()(address, val.values));
      }
    }

  private:
    OscSender m_sender;
    LockedParameterMap<ParameterMap> m_map;
};
}
}
