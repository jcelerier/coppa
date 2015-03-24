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

// Cases : fully static (midi), non-queryable (pure osc), queryable (minuit, oscquery)
class RemoteDevice
{
    OscSender m_sender;
    LockedParameterMap<ParameterMap> m_map;
    WebSocketClient m_client{
      [&] (auto&&... args)
      { onMessage(std::forward<decltype(args)>(args)...); }
    };

    std::string m_serverURI;

    void onMessage(WebSocketClient::connection_handler hdl, const std::string& message)
    try
    {
      if(message.empty())
        return;

      switch(JSONParser::messageType(message))
      {
        case MessageType::Device:
          m_sender = OscSender{m_serverURI, JSONParser::getPort(message)};
          break;
        case MessageType::PathAdded:
          break;
        case MessageType::PathRemoved:
          // TODO differentiate between removing a parameter,
          // and removing a whole part of the tree

          break;
        case MessageType::PathChanged:
          // Pass the map for modification
          JSONParser::parsePathChanged(m_map, message);
          break;
        default:
          m_map = JSONParser::parseNamespace<ParameterMap>(message);
          break;
      }
    }
    catch(BadRequestException& e)
    {
      std::cerr << "Error while parsing: " << e.what() << "  ==>  " << message;
    }

  public:
    RemoteDevice(const std::string& uri):
      m_serverURI{m_client.connect(uri)}
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
}
}
