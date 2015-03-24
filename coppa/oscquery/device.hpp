#pragma once
#include <coppa/oscquery/websockets.hpp>
#include <coppa/osc/oscreceiver.hpp>
#include <coppa/osc/oscsender.hpp>
#include <coppa/osc/oscmessagegenerator.hpp>
#include <unordered_set>

namespace coppa
{
namespace oscquery
{
// Separate the protocol (websockets), its serialization format OSCQueryJsonFormat
// Local device : websockets query server, osc socket (?), OSCQueryParameterMap

// todo If there are two clients and one changes the value, the other must be updated.
// If the value is changed locally, the clients must be updated
template<typename QueryServer>
class LocalDevice
{
    class RemoteClient
    {
        typename QueryServer::connection_handler m_handler;
        std::unordered_set<std::string> m_listened;

      public:
        RemoteClient(typename QueryServer::connection_handler handler):
          m_handler{handler} { }

        operator typename QueryServer::connection_handler() const
        { return m_handler; }

        // For performance's sake, it would be better
        // to revert this and have a table of client id's associated to each listened parameters.
        void addListenedPath(const std::string& path)
        { m_listened.insert(path); }
        void removeListenedPath(const std::string& path)
        { m_listened.erase(path); }

        const auto& listenedPaths() const noexcept
        { return m_listened; }

        bool operator==(const typename QueryServer::connection_handler& h) const
        { return !m_handler.expired() && m_handler.lock() == h.lock(); }
    };

    template<typename Attribute>
    void updateRemoteAttribute(const RemoteClient& clt, const std::string& path, const Attribute& attr)
    {
      std::string message = JSONFormat::attributeChangedMessage(path, attr);

      m_server.sendMessage(clt, message);
    }

    std::string generateReply(typename QueryServer::connection_handler hdl,
                              const std::string& request)
    {
      using namespace boost;
      std::string response;
      if(algorithm::contains(request, "?"))
      {
        std::vector<std::string> tokens;
        boost::split(tokens, request, boost::is_any_of("?"));
        if(tokens.size() != 2)
        {
          std::cerr << "Invalid request: " << request << std::endl;
          return "";
        }

        auto path = tokens.at(0);
        auto method = tokens.at(1);
        if(algorithm::contains(method, "listen"))
        {
          std::vector<std::string> listen_tokens;
          boost::split(listen_tokens, method, boost::is_any_of("="));
          auto it = std::find(std::begin(m_clients), std::end(m_clients), hdl);
          if(it == std::end(m_clients))
            return "";

          if(listen_tokens.size() != 2)
          {
            std::cerr << "Invalid request: " << request << std::endl;
            return "";
          }

          if(listen_tokens.at(1) == "true")
            it->addListenedPath(path);
          else
            it->removeListenedPath(path);
        }
        else
        {
          std::lock_guard<std::mutex> lock(m_map_mutex);
          response = JSONFormat::marshallAttribute(
                       *m_map.get<0>().find(path),
                       method);
        }
      }
      else
      {
        std::lock_guard<std::mutex> lock(m_map_mutex);
        response = JSONFormat::marshallParameterMap(m_map, request);
      }

      return response;
    }


    mutable std::mutex m_map_mutex;
    ParameterMap m_map;
    std::vector<RemoteClient> m_clients;
    QueryServer m_server
    {
      // Open handler
      [&] (typename QueryServer::connection_handler hdl)
      {
        m_clients.emplace_back(hdl);

        // Send the client a message with the OSC port
        m_server.sendMessage(hdl, JSONFormat::deviceInfo(m_receiver.port()));
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
        return generateReply(hdl, message);
      }
    };

    OscReceiver m_receiver{1234};
  public:
    LocalDevice()
    {
      Parameter root;
      root.description = std::string("root node");
      root.destination = std::string("/");
      root.accessmode = AccessMode::None;
      m_map.insert(root);
    }

    void add(const Parameter& parameter)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.insert(parameter);
      auto addMessage = JSONFormat::insertParameterMessage(parameter);

      for(auto& client : m_clients)
      {
        m_server.sendMessage(client, addMessage);
      }
    }

    void remove(const std::string& path)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.get<0>().erase(path);

      auto removeMessage =  JSONFormat::removePathMessage(path);
      for(auto& client : m_clients)
      {
        m_server.sendMessage(client, removeMessage);
      }
    }

    template<typename Attribute>
    void update(const std::string& path, const Attribute& val)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      auto& param_index = m_map.get<0>();
      decltype(auto) param = param_index.find(path);
      param_index.modify(param, [=] (Parameter& p) { static_cast<Attribute&>(p) = val; });

      for(auto& client : m_clients)
      {
        updateRemoteAttribute(client, path, val);
      }
    }

    void rename(std::string oldPath, std::string newPath)
    { /* todo PATH_CHANGED */ }

    Parameter get(const std::string& address) const
    { return *m_map.get<0>().find(address); }

    ParameterMap map() const
    { return m_map; }


    void expose()
    { m_server.run(); }
};


// Cases : fully static (midi), non-queryable (pure osc), queryable (minuit, oscquery)
class RemoteDevice
{
    mutable std::mutex m_map_mutex;
    OscSender m_sender;
    ParameterMap m_map;
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

      switch(JSONRead::messageType(message))
      {
        case MessageType::Device:
          m_sender = OscSender{m_serverURI, JSONRead::getPort(message)};
          break;
        case MessageType::PathAdded:
          break;
        case MessageType::PathRemoved:

          // TODO differentiate between removing a parameter,
          // and removing a whole part of the tree

          break;
        case MessageType::PathChanged:
          break;
        default:
          break;
          // namespace ?
          //throw BadRequestException{};
      }


      json_map obj{ message };
      if(obj.find("path_changed") != obj.end())
      {
        std::string path = JSONRead::jsonToString(obj.get("path_changed"));

        // A small lambda to modify the parameter map easily with locking
        auto mapper = [&] (const std::string& name, auto&& getter, auto&& method)
        {
          if(obj.find(name) != obj.end())
          {
            auto& param_index = m_map.get<0>();
            decltype(auto) param = param_index.find(path);
            {
              std::lock_guard<std::mutex> lock(m_map_mutex);
              param_index.modify(param, [&] (Parameter& p) { getter(p) = method(obj.get(name)); });
            }
          }
          else
          {
            throw BadRequestException{};
          }
        };

        mapper("description", [] (auto&& p) -> auto& { return p.description; }, &JSONRead::jsonToString);
        mapper("tags", [] (auto&& p) -> auto& { return p.tags; }, &JSONRead::jsonToTags);
        mapper("value", [] (auto&& p) -> auto& { return p.values; }, &JSONRead::jsonToValueArray);
        mapper("range", [] (auto&& p) -> auto& { return p.ranges; }, &JSONRead::jsonToRangeArray);
        mapper("clipmode", [] (auto&& p) -> auto& { return p.clipmodes; }, &JSONRead::jsonToClipModeArray);
        mapper("access", [] (auto&& p) -> auto& { return p.accessmode; }, &JSONRead::jsonToAccessMode);
      }
      else
      {
        std::cout << "Message: " << message << std::endl;

        // Parse json. For now only the whole namespace.
        auto newMap = JSONRead::toMap(message);
        {
          std::lock_guard<std::mutex> lock(m_map_mutex);
          m_map = std::move(newMap);
        }
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
      std::lock_guard<std::mutex> lock(m_map_mutex);
      decltype(auto) index = m_map.get<0>();
      return index.find(address) != end(index);
    }

    // Get the local value
    Parameter get(const std::string& address) const
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      return *m_map.get<0>().find(address);
    }

    ParameterMap map() const
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      return m_map;
    }

    // Network operations
    void set(const std::string& address, const Values& val)
    {
      // Update local and send a message via OSC
      // Parameter must be settable
      auto param = get(address);
      if(param.accessmode == AccessMode::Set
         || param.accessmode == AccessMode::Both)
      {
        m_sender.send(osc::MessageGenerator()(address, val.values));
      }
    }

    // Ask for an update of a part of the namespace
    void update(const std::string& root = "/")
    {
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
