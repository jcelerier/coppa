#pragma once
#include <coppa/coppa.hpp>
#include <jeayeson/jeayeson.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/http/request.hpp>

#include <chrono>

namespace coppa
{
    namespace oscquery
    {
        struct Range
        {
                boost::optional<Variant> min;
                boost::optional<Variant> max;
                std::vector<Variant> values;
        };

        struct Value
        {
                Variant value;
                Range range;
                ClipMode clipMode{ClipMode::None};
        };

        struct Values
        {
                coppa_name(Values)
                std::vector<oscquery::Value> values;
        };

        using Parameter =
        ParameterAdapter<
        Values,
        Description,
        Tags,
        Alias,
        RepetitionFilter,
        Bounds<Variant,
        StandardComparator<Variant>,
        StandardComparator<Variant>>>;

        using ParameterMap = ParameterMapType<Parameter>;

        class JSONFormat
        {
            public:
                // Format interface
                template<typename... Args>
                static std::string marshallParameterMap(Args&&... args)
                {
                    return mapToJson(std::forward<Args>(args)...).to_string();
                }

                template<typename... Args>
                static std::string marshallAttribute(Args&&... args)
                {
                    return attributeToJson(std::forward<Args>(args)...).to_string();
                }

            private:
                static std::string getJsonTypeString(const Parameter& parameter)
                {
                    std::string str_type;
                    for(const auto& oscqvalue : parameter.values)
                    {
                        const auto& value = oscqvalue.value;
                        switch(value.which())
                        {
                            case 0: str_type += "i"; break;
                            case 1: str_type += "f"; break;
                                // case 2: str_type += "B"; break; -> no bool
                            case 3: str_type += "s"; break;
                            case 4: str_type += "b"; break;
                        }
                    }

                    return str_type;
                }

                static void addValueToJsonArray(json_array& array, const Variant& val)
                {
                    using namespace eggs::variants;
                    switch(val.which())
                    {
                        case 0: array.add(get<int>(val)); break;
                        case 1: array.add(get<float>(val)); break;
                            //case 2: array.add(get<bool>(val)); break;
                        case 3: array.add(get<std::string>(val)); break;
                        case 4: array.add(get<const char*>(val)); break;
                    }
                }

                static json_array getJsonValueArray(const Parameter& parameter)
                {
                    json_array value_arr;
                    for(const auto& oscqvalue : parameter.values)
                    {
                        addValueToJsonArray(value_arr, oscqvalue.value);
                    }

                    return value_arr;
                }

                static json_array getJsonClipModeArray(const Parameter& parameter)
                {
                    json_array clip_arr;
                    for(const auto& oscqvalue : parameter.values)
                    {
                        switch(oscqvalue.clipMode)
                        {
                            case ClipMode::None: clip_arr.add("None"); break;
                            case ClipMode::Low:  clip_arr.add("Low");  break;
                            case ClipMode::High: clip_arr.add("High"); break;
                            case ClipMode::Both: clip_arr.add("Both"); break;
                        }
                    }

                    return clip_arr;
                }

                static json_array getJsonRangeArray(const Parameter& parameter)
                {
                    json_array range_arr;
                    for(const auto& oscqvalue : parameter.values)
                    {
                        json_array range_subarray;
                        if(!oscqvalue.range.min)
                        { range_subarray.add("null"); }
                        else
                        { addValueToJsonArray(range_subarray, *oscqvalue.range.min); }

                        if(!oscqvalue.range.max)
                        { range_subarray.add("null"); }
                        else
                        { addValueToJsonArray(range_subarray, *oscqvalue.range.max); }

                        if(oscqvalue.range.values.empty())
                        { range_subarray.add("null"); }
                        else
                        {
                            json_array range_values_array;
                            for(auto& elt : oscqvalue.range.values)
                            {
                                addValueToJsonArray(range_values_array, elt);
                            }
                            range_subarray.add(range_values_array);
                        }

                        range_arr.add(range_subarray);
                    }

                    return range_arr;
                }

                static json_array getJsonTags(const Parameter& parameter)
                {
                    json_array arr;
                    for(const auto& tag : parameter.tags)
                        arr.add(tag);

                    return arr;
                }

                static json_map attributeToJson(const Parameter& parameter, std::string method)
                {
                    json_map map;

                    if(method == "value")
                    { map.set("value", getJsonValueArray(parameter)); }
                    else if(method == "range")
                    { map.set("range", getJsonRangeArray(parameter)); }
                    else if(method == "clipmode")
                    { map.set("clipmode", getJsonClipModeArray(parameter)); }
                    else if(method == "access")
                    { map.set("access", static_cast<int>(parameter.accessMode)); }
                    else if(method == "type")
                    { map.set("type", getJsonTypeString(parameter)); }
                    else if(method == "description")
                    { map.set("description", parameter.description); }
                    else if(method == "tags")
                    { map.set("value", getJsonTags(parameter)); }
                    else if(method == "full_path")
                    { map.set("full_path", parameter.destination); }

                    return map;
                }

                static void parameterToJson(const Parameter& parameter,
                                     json_map& obj)
                {
                    using namespace std;
                    using namespace boost;
                    using namespace eggs::variants;

                    // These attributes are always here
                    obj.set("full_path", parameter.destination);
                    obj.set("access", static_cast<int>(parameter.accessMode));

                    // Potentially empty attributes :
                    // Description
                    if(!parameter.description.empty())
                    {
                        obj.set("description", parameter.description);
                    }

                    // Tags
                    if(!parameter.tags.empty())
                    {
                        obj.set("tags", getJsonTags(parameter));
                    }

                    // Handling of the types / values
                    if(!parameter.values.empty())
                    {
                        obj.set("type", getJsonTypeString(parameter));
                        obj.set("value", getJsonValueArray(parameter));
                        obj.set("clipmode", getJsonClipModeArray(parameter));
                        obj.set("range", getJsonRangeArray(parameter));
                    }
                }

                // A ParameterMap can be JSON'd
                static json_map mapToJson(const ParameterMap& map, std::string root)
                {
                    using namespace std;
                    using namespace boost;
                    using namespace eggs::variants;
                    // Root node
                    json_map localroot;

                    // Create a tree with the parameters
                    for(const auto& parameter : filter(map, root))
                    {
                        // Trunk the given root from the parameters
                        auto trunked_dest = parameter.destination;
                        if(root != "/")
                            trunked_dest.erase(0, root.length());

                        char_separator<char> sep("/");
                        tokenizer<char_separator<char>> tokens(trunked_dest, sep);

                        // Create the required parts of the tree and navigate to the corresponding node
                        auto* current_map = &localroot;
                        for(const auto& token : tokens)
                        {
                            if(!current_map->has("contents"))
                            { current_map->set("contents", json_map{}); }

                            current_map = &current_map->get_for_path<json_map>("contents");

                            if(!current_map->has(token))
                            { current_map->set(token, json_map{}); }

                            current_map = &current_map->get_for_path<json_map>(token);
                        }

                        parameterToJson(parameter, *current_map);
                    }

                    return localroot;
                }
        };

        class WebSocketServer
        {
                ParameterMap& m_map;

            public:
                using server = websocketpp::server<websocketpp::config::asio>;
                WebSocketServer(ParameterMap& map):
                    m_map{map}
                {
                    using namespace websocketpp::lib;

                    m_server.init_asio();

                    m_server.set_open_handler(bind(&WebSocketServer::on_open,this,placeholders::_1));
                    m_server.set_close_handler(bind(&WebSocketServer::on_close,this,placeholders::_1));
                    m_server.set_message_handler(bind(&WebSocketServer::on_message,this,placeholders::_1, placeholders::_2));
                    m_server.set_http_handler(bind(&WebSocketServer::on_http,this,placeholders::_1));
                }

                void on_open(websocketpp::connection_hdl hdl)
                {
                    m_connections.insert(hdl);
                }

                void on_close(websocketpp::connection_hdl hdl)
                {
                    m_connections.erase(hdl);
                }

                void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
                {
                    server::connection_ptr con = m_server.get_con_from_hdl(hdl);
                    con->send(generate_reply(msg->get_payload()));
                }

                void on_http(websocketpp::connection_hdl hdl)
                {
                    server::connection_ptr con = m_server.get_con_from_hdl(hdl);
                    con->set_body(generate_reply(con->get_uri()->get_resource()));
                    con->set_status(websocketpp::http::status_code::ok);
                }

                void run(uint16_t port = 9002)
                {
                    m_server.listen(port);
                    m_server.start_accept();
                    m_server.run();
                }

                std::string generate_reply(std::string request)
                {
                    using namespace boost;
                    std::string response;
                    if(algorithm::contains(request, "?"))
                    {
                        char_separator<char> sep("?");
                        tokenizer<char_separator<char>> tokens(request, sep);

                        std::string address, method;
                        int i = 0;
                        for(const auto& token : tokens)
                        {
                            if(i++ == 0) address = token;
                            else method = token;
                        }

                        response = JSONFormat::marshallAttribute(*m_map.get<0>().find(address), method);
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

            private:
                using con_list = std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>>;

                server m_server;
                con_list m_connections;
        };


        // Separate the protocol (websockets), its serialization format OSCQueryJsonFormat
        // Local device : websockets query server, osc socket (?), OSCQueryParameterMap

        class LocalDevice
        {
                ParameterMap m_map;
                WebSocketServer m_server;

            public:
                LocalDevice():
                    m_server{m_map}
                {
                    Parameter root;
                    root.description = std::string("root node");
                    root.destination = std::string("/");
                    root.accessMode = AccessMode::None;
                    m_map.insert(root);
                }

                void add(const Parameter& parameter)
                { m_map.insert(parameter); }

                void remove(const std::string& path)
                { m_map.get<0>().erase(path); }

                template<typename Attribute>
                void update(std::string path, const Attribute& val)
                { m_map.get<0>().find(path)->get<Attribute>() = val;  }

                Parameter get(const std::string& address) const
                { return *m_map.get<0>().find(address); }

                ParameterMap map() const
                { return m_map; }


                void expose()
                { m_server.run(); }
        };

        class WebSocketClient
        {
                websocketpp::lib::thread asio_thread;
            public:
                typedef websocketpp::client<websocketpp::config::asio_client> client;
                typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;

                WebSocketClient() :
                    m_open(false),
                    m_done(false)
                {
                    using namespace websocketpp::lib;
                    using namespace std::placeholders;

                    m_client.clear_access_channels(websocketpp::log::alevel::all);
                    m_client.init_asio();

                    m_client.set_open_handler(bind(&WebSocketClient::on_open, this, ::_1));
                    m_client.set_message_handler(bind(&WebSocketClient::on_message, this, ::_1, ::_2));
                }

                ~WebSocketClient()
                {
                    if(m_open)
                        stop();

                    if(asio_thread.joinable())
                        asio_thread.join();
                }

                void on_open(websocketpp::connection_hdl hdl)
                {
                    scoped_lock guard(m_lock);
                    m_open = true;
                }

                void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg)
                {
                    std::cout << std::endl << std::endl << msg->get_payload() << std::endl << std::endl;
                }

                void stop()
                {
                    scoped_lock guard(m_lock);
                    m_client.close(m_hdl, websocketpp::close::status::normal, "");
                    m_open = false;
                }

                void run(const std::string & uri)
                {
                    websocketpp::lib::error_code ec;
                    auto con = m_client.get_connection(uri, ec);
                    if (ec)
                    {
                        m_client.get_alog().write(websocketpp::log::alevel::app,
                                                  "Get Connection Error: " + ec.message());
                        return;
                    }

                    m_hdl = con->get_handle();
                    m_client.connect(con);

                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    asio_thread = websocketpp::lib::thread{&client::run, &m_client};
                }


                void send_request(const std::string& request)
                {
                    if(!m_open)
                        return;

                    websocketpp::lib::error_code ec;
                    m_client.send(m_hdl, request, websocketpp::frame::opcode::text, ec);

                    if (ec)
                    {
                        m_client.get_alog().write(websocketpp::log::alevel::app,
                                                  "Send Error: "+ec.message());
                    }
                }

            private:
                client m_client;
                websocketpp::connection_hdl m_hdl;
                websocketpp::lib::mutex m_lock;
                bool m_open;
                bool m_done;
        };

        class RemoteDevice
        {
                ParameterMap m_map;
                WebSocketClient m_client;

            public:
                RemoteDevice(std::string uri)
                {
                    m_client.run(uri);
                }

                // Get the local value
                Parameter get(const std::string& address) const
                {
                    return *m_map.get<0>().find(address);
                }

                ParameterMap map() const
                { return m_map; }

                // Network operations
                void set(const std::string& address) const
                {
                    // Update local and send a message
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

                void listenNamespace(bool)
                {

                }

                void listenParameter(bool)
                {

                }

        };
    }

}
