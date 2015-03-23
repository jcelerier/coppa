#pragma once
#include <coppa/oscquery/websockets.hpp>
#include <unordered_set>

namespace coppa
{
    namespace oscquery
    {
        // Separate the protocol (websockets), its serialization format OSCQueryJsonFormat
        // Local device : websockets query server, osc socket (?), OSCQueryParameterMap

        // todo If there are two clients and one changes the value, the other must be updated.
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
                        { m_listened.insert(path); std::cout << path << " added\n";}
                        void removeListenedPath(const std::string& path)
                        { m_listened.erase(path);  std::cout << path << " removed\n";}

                        const auto& listenedPaths() const noexcept
                        { return m_listened; }

                        bool operator==(const typename QueryServer::connection_handler& h) const
                        { return !m_handler.expired() && m_handler.lock() == h.lock(); }
                };

                template<typename Attribute>
                void updateRemoteAttribute(const RemoteClient& clt, Parameter& param, const Attribute& attr)
                {
                    std::string message;
                    // todo Make a path_changed json object.

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
                            response = JSONFormat::marshallAttribute(
                                           *m_map.get<0>().find(path),
                                           method);
                        }
                    }
                    else
                    {
                        response = JSONFormat::marshallParameterMap(m_map, request);
                        /* TODO
                        if(requested_path == "/")
                        {
                            json_map device_map;
                            device_map.set("name", "Pretty Device Name");
                            device_map.set("port", 12345);

                            map.set("device_parameters", device_map);
                        }*/
                    }

                    return response;
                }

                ParameterMap m_map;
                std::vector<RemoteClient> m_clients;
                QueryServer m_server
                {
                    // Open handler
                    [&] (typename QueryServer::connection_handler hdl)
                    {
                        m_clients.emplace_back(hdl);
                        std::cout << "client added\n";
                    },
                    // Close handler
                    [&] (typename QueryServer::connection_handler hdl)
                    {
                       auto it = std::find(begin(m_clients), end(m_clients), hdl);
                       if(it != end(m_clients))
                       {
                           m_clients.erase(it);
                           std::cout << "client removed\n";
                       }
                    },
                    // Message handler
                    [&] (typename QueryServer::connection_handler hdl, const std::string& message)
                    {
                       return generateReply(hdl, message);
                    }
                };

            public:
                LocalDevice()
                {
                    Parameter root;
                    root.description = std::string("root node");
                    root.destination = std::string("/");
                    root.accessMode = AccessMode::None;
                    m_map.insert(root);
                }

                void add(const Parameter& parameter)
                {
                    m_map.insert(parameter);
                    auto addMessage = JSONFormat::insertParameterMessage(parameter);

                    for(auto& client : m_clients)
                    {
                        m_server.sendMessage(client, addMessage);
                    }
                }

                void remove(const std::string& path)
                {
                    m_map.get<0>().erase(path);

                    auto removeMessage =  JSONFormat::removePathMessage(path);
                    for(auto& client : m_clients)
                    {
                        m_server.sendMessage(client, removeMessage);
                    }
                }

                template<typename Attribute>
                void update(std::string path, const Attribute& val)
                {
                    auto& attr = m_map.get<0>().find(path)->template get<Attribute>();
                    attr = val;
                    for(auto& client : m_clients)
                    {
                        // If the attribute is the value, it should go by the data protocol (OSC)
                        // Else, it should go by the query protocol (WebSockets)
                        updateRemoteAttribute(client,
                                              m_map.get<0>().find(path),
                                              attr);
                        break; // It can be only once

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
                ParameterMap m_map;
                WebSocketClient m_client;

            public:
                RemoteDevice(std::string uri)
                {
                    m_client.connect(uri);
                }

                // Get the local value
                Parameter get(const std::string& address) const
                {
                    return *m_map.get<0>().find(address);
                }

                ParameterMap map() const
                { return m_map; }

                // Network operations
                template<typename Val>
                void set(const std::string& address, Val&& val) const
                {
                    // Update local and send a message
                    // Parameter must be settable
                }

                // Ask for an update of a part of the namespace
                void update(const std::string& root = "/")
                {
                    m_client.send_request(root);
                }

                // Ask for an update of a single attribute
                void updateAttribute(const std::string& address, const std::string& attribute)
                {
                    m_client.send_request(address + "?" + attribute);
                }

                void listenAddress(const std::string& address, bool b)
                {
                    m_client.send_request(address + "?listen=" + (b? "true" : "false"));
                }
        };
    }
}
