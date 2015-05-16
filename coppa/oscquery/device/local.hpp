#pragma once
#include <coppa/exceptions/BadRequest.hpp>
#include <coppa/oscquery/websockets.hpp>
#include <coppa/protocol/osc/oscreceiver.hpp>
#include <coppa/protocol/osc/oscsender.hpp>
#include <coppa/protocol/osc/oscmessagegenerator.hpp>
#include <unordered_set>
#include <map>
#include <coppa/oscquery/map.hpp>
#include <coppa/oscquery/json/writer.hpp>

class QueryParser
{
    // The query are similar to the GET part of an http request.
  public:
    template<typename Mapper>
    static std::string parse(const std::string& request, Mapper&& mapper)
    {
      using namespace boost;
      using namespace std;
      auto invalid_request = [&] ()
      {
        cerr << "Invalid request: " << request << endl;
        return "";
      };

      // First split the "?" part
      vector<string> uri_tokens;
      split(uri_tokens, request, is_any_of("?"));

      string path = uri_tokens.at(0);
      map<string, string> arguments_map;

      if(uri_tokens.size() > 1)
      {
        if(uri_tokens.size() > 2)
        {
          return invalid_request();
        }

        auto arguments = uri_tokens.at(1);

        // Then, split the &-separated arguments
        vector<string> argument_tokens;
        split(argument_tokens, arguments, is_any_of("&"));

        // Finally, split these arguments at '=' at put them in a map
        for(const auto& arg : argument_tokens)
        {
          vector<string> map_tokens;
          split(map_tokens, arg, is_any_of("="));

          switch(map_tokens.size())
          {
            case 2:
              arguments_map.insert({map_tokens.front(), map_tokens.back()});
              break;
            case 1:
              arguments_map.insert({map_tokens.front(), ""});
              break;
            default:
              return invalid_request();
              break;
          }
        }
      }

      return mapper(path, arguments_map);
    }

};

namespace coppa
{
namespace oscquery
{


template<typename QueryServer>
class RemoteClient
{
    typename QueryServer::connection_handler m_handler;
    std::unordered_set<std::string> m_listened;

  public:
    RemoteClient(
        typename QueryServer::connection_handler handler):
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

    bool operator==(
        const typename QueryServer::connection_handler& h) const
    { return !m_handler.expired() && m_handler.lock() == h.lock(); }
};
// Separate the protocol (websockets), its serialization format OSCQueryJsonFormat
// Local device : websockets query server, osc socket (?), OSCQueryParameterMap

// todo If there are two clients and one changes the value, the other must be updated.
// If the value is changed locally, the clients must be updated
template<typename QueryServer>
class LocalDevice
{
  private:
    // Should be in a protocol-specific class.
    std::string generateReply(
        typename QueryServer::connection_handler hdl,
        const std::string& request)
    {
      using namespace std;
      return QueryParser::parse(
            request,
            [&] (const string& path, const std::map<string, string>& parameters)
      {
        std::lock_guard<std::mutex> lock(m_map_mutex);
        // Here we handle the url elements relative to oscquery
        if(parameters.size() == 0)
        {
          return JSON::writer::query_namespace(m_map.unsafeMap(), path);
        }
        else
        {
          // First check if we have the path
          if(!map().has(path))
            throw BadRequestException{"Path not found"};

          // Listen
          auto listen_it = parameters.find("listen");
          if(listen_it != end(parameters))
          {
            // First we find for a corresponding client
            auto it = find(begin(m_clients), end(m_clients), hdl);
            if(it == std::end(m_clients))
              throw BadRequestException{"Client not found"};

            // Then we enable / disable listening
            if(listen_it->second == "true")
            {
              it->addListenedPath(path);
            }
            else if(listen_it->second == "false")
            {
              it->removeListenedPath(path);
            }
            else
            {
              throw BadRequestException{};
            }
          }

          // All the value-less parameters
          std::vector<std::string> attributes;
          for(const auto& elt : parameters)
          {
            if(elt.second.empty())
            {
              attributes.push_back(elt.first);
            }
          }

          if(!attributes.empty())
          {
            return JSON::writer::query_attributes(
                  m_map.unsafeMap().get(path),
                  attributes);
          }
        }

        return std::string{};
      });
    }


  public:
    const auto& clients() const
    {
      return m_clients;
    }
    auto& server()
    {
      return m_server;
    }

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
          return this->generateReply(hdl, message);
      }
    };
};

// Automatically synchronizes its changes with the client.
// Note : put somewhere if the client wants to be synchronized
// Note : be careful with the locking too, here
template<typename QueryServer>
class SynchronizingLocalDevice
{
  private:
    LocalDevice<QueryServer> m_device;

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
