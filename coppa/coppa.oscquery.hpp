#pragma once
#include <coppa/coppa.hpp>
#include <jeayeson/jeayeson.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/http/request.hpp>

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

        namespace json
        {
            std::string getJsonTypeString(const Parameter& parameter)
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

            void addValueToJsonArray(json_array& array, const Variant& val)
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

            json_array getJsonValueArray(const Parameter& parameter)
            {
                json_array value_arr;
                for(const auto& oscqvalue : parameter.values)
                {
                    addValueToJsonArray(value_arr, oscqvalue.value);
                }

                return value_arr;
            }

            json_array getJsonClipModeArray(const Parameter& parameter)
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

            json_array getJsonRangeArray(const Parameter& parameter)
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

            json_array getJsonTags(const Parameter& parameter)
            {
                json_array arr;
                for(const auto& tag : parameter.tags)
                    arr.add(tag);

                return arr;
            }

            void parameterToJson(const Parameter& parameter,
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
            json_map mapToJson(const ParameterMap& map, std::string root)
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
        }

        class WebSocketServer
        {
                ParameterMap& m_map;
            public:
                std::string getMethod(std::string address, std::string method)
                {
                    using namespace json;
                    const auto& parameter = *m_map.get<0>().find(address);
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

                    return map.to_string();
                }

                using server = websocketpp::server<websocketpp::config::asio>;
                void on_http(websocketpp::connection_hdl hdl)
                {
                    using namespace boost;
                    server::connection_ptr con = m_server.get_con_from_hdl(hdl);

                    std::string requested_path = con->get_uri()->get_resource();
                    if(algorithm::contains(requested_path, "?"))
                    {
                        char_separator<char> sep("?");
                        tokenizer<char_separator<char>> tokens(requested_path, sep);

                        std::string address;
                        std::string method;
                        int i = 0;
                        for(auto& token : tokens)
                        {
                            if(i++ == 0) address = token;
                            else method = token;
                        }
                        std::cout << address << " " << method << std::endl;

                        con->set_body(getMethod(address, method));
                    }
                    else
                    {
                        con->set_body(json::mapToJson(m_map, requested_path).to_string());
                    }

                    con->set_status(websocketpp::http::status_code::ok);
                }

                WebSocketServer(ParameterMap& map):
                    m_map{map}
                {
                    using namespace websocketpp::lib;
                    Parameter root;
                    root.description = std::string("root node");
                    root.destination = std::string("/");
                    root.accessMode = AccessMode::None;
                    m_map.insert(root);

                    m_server.init_asio();

                    m_server.set_open_handler(bind(&WebSocketServer::on_open,this,placeholders::_1));
                    m_server.set_close_handler(bind(&WebSocketServer::on_close,this,placeholders::_1));
                    m_server.set_message_handler(bind(&WebSocketServer::on_message,this,placeholders::_1, placeholders::_2));
                    m_server.set_http_handler(bind(&WebSocketServer::on_http,this,placeholders::_1));
                }

                ~WebSocketServer()
                {
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
                    for (auto it : m_connections)
                    {
                        m_server.send(it,msg);
                    }
                }

                void run(uint16_t port)
                {
                    m_server.listen(port);
                    m_server.start_accept();
                    m_server.run();
                }

            private:
                typedef std::set<websocketpp::connection_hdl,
                std::owner_less<websocketpp::connection_hdl>> con_list;

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
                    // Set-up a query server and osc server
                    // Keep track of the listening clients
                    // Local changes (from get/set methods here) should be pushed to them
                }

                void add(const Parameter& parameter)
                {
                    m_map.insert(parameter);
                }

                void remove(const std::string& path)
                {
                    m_map.get<0>().erase(path);
                }

                template<typename Attribute>
                void update(std::string path, const Attribute& val)
                {
                    m_map.get<0>().find(path)->get<Attribute>() = val;

                    // Todo : update
                }
        };
    }

}
