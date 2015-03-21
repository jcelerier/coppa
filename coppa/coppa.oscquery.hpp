#pragma once
#include <coppa/coppa.hpp>
#include <jeayeson/jeayeson.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/http/request.hpp>
namespace coppa
{

    struct Range
    {
            boost::optional<Variant> min;
            boost::optional<Variant> max;
            std::vector<Variant> values;
    };

    struct OSCQueryValue
    {
            Variant value;
            Range range;
            ClipMode clipMode{ClipMode::None};
    };

    class Values
    {
            coppa_name(Values)
            public:
                std::vector<OSCQueryValue> values;
    };


    using OSCQueryParameter = ParameterAdapter<
    Values,
    Description,
    Tags,
    Alias,
    RepetitionFilter,
    Bounds<Variant,
    StandardComparator<Variant>,
    StandardComparator<Variant>>>;

    using OSCQueryParameterMap = ParameterMapType<OSCQueryParameter>;

    OSCQueryParameterMap filter(const OSCQueryParameterMap& map, std::string addr)
    {
        using namespace std;
        OSCQueryParameterMap newmap;
        for(const OSCQueryParameter& param : map)
        {
            if(boost::starts_with(param.destination, addr))
                newmap.insert(param);
        }

        return newmap;
    }

    std::string getJsonTypeString(const OSCQueryParameter& parameter)
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

    json_array getJsonValueArray(const OSCQueryParameter& parameter)
    {
        json_array value_arr;
        for(const auto& oscqvalue : parameter.values)
        {
            addValueToJsonArray(value_arr, oscqvalue.value);
        }

        return value_arr;
    }

    json_array getJsonClipModeArray(const OSCQueryParameter& parameter)
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

    json_array getJsonRangeArray(const OSCQueryParameter& parameter)
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

    json_array getJsonTags(const OSCQueryParameter& parameter)
    {
        json_array arr;
        for(const auto& tag : parameter.tags)
            arr.add(tag);

        return arr;
    }

    void parameterToJson(const OSCQueryParameter& parameter,
                         json_map* current_map)
    {
        using namespace std;
        using namespace boost;
        using namespace eggs::variants;

        // These attributes are always here
        current_map->set("full_path", parameter.destination);
        current_map->set("access", static_cast<int>(parameter.accessMode));

        // Potentially empty attributes :
        // Description
        if(!parameter.description.empty())
        {
            current_map->set("description", parameter.description);
        }

        // Tags
        if(!parameter.tags.empty())
        {
            current_map->set("tags", getJsonTags(parameter));
        }

        // Handling of the types / values
        if(!parameter.values.empty())
        {
            current_map->set("type", getJsonTypeString(parameter));
            current_map->set("value", getJsonValueArray(parameter));
            current_map->set("clipmode", getJsonClipModeArray(parameter));
            current_map->set("range", getJsonRangeArray(parameter));
        }
    }

    // A ParameterMap can be JSON'd
    json_map toJson(const OSCQueryParameterMap& map, std::string root)
    {
        using namespace std;
        using namespace boost;
        using namespace eggs::variants;
        // Root node
        json_map localroot;

        // Create a tree with the parameters
        for(const auto& parameter : filter(map, root))
        {
            auto trunked_dest = parameter.destination;

            // /a/b/c
            // root = /a
            // trunked = /b/c
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

           parameterToJson(parameter, current_map);
        }

        return localroot;
    }

    class OSCQueryServer
    {
            OSCQueryParameterMap m_map;
        public:
            std::string getMethod(std::string address, std::string method)
            {
                const auto& parameter = *m_map.get<0>().find(address);
                json_map map;

                if(method == "value")
                {
                    map.set("value", getJsonValueArray(parameter));
                }
                else if(method == "range")
                {
                    map.set("range", getJsonRangeArray(parameter));
                }
                else if(method == "clipmode")
                {
                    map.set("clipmode", getJsonClipModeArray(parameter));
                }
                else if(method == "access")
                {
                    map.set("access", static_cast<int>(parameter.accessMode));
                }
                else if(method == "type")
                {
                    map.set("type", getJsonTypeString(parameter));
                }
                else if(method == "description")
                {
                    map.set("description", parameter.description);
                }
                else if(method == "tags")
                {
                    map.set("value", getJsonTags(parameter));
                }
                else if(method == "full_path")
                {
                    map.set("full_path", parameter.destination);
                }

                return map.to_string();
            }

            using server = websocketpp::server<websocketpp::config::asio>;
            void on_http(websocketpp::connection_hdl hdl)
            {
                using namespace boost;
                server::connection_ptr con = m_server.get_con_from_hdl(hdl);

                std::string requested_path = con->get_uri()->get_resource();
                std::cout << con->get_request().raw();
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
                    con->set_body(toJson(m_map, requested_path).to_string());
                }

                con->set_status(websocketpp::http::status_code::ok);
            }

            OSCQueryServer(OSCQueryParameterMap map):
                m_map{map}
            {
                OSCQueryParameter root;
                root.description = std::string("root node");
                root.destination = std::string("/");
                root.accessMode = AccessMode::None;
                m_map.insert(root);

                m_server.init_asio();

                m_server.set_open_handler(websocketpp::lib::bind(&OSCQueryServer::on_open,this,websocketpp::lib::placeholders::_1));
                m_server.set_close_handler(websocketpp::lib::bind(&OSCQueryServer::on_close,this,websocketpp::lib::placeholders::_1));
                m_server.set_message_handler(websocketpp::lib::bind(&OSCQueryServer::on_message,this,websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
                m_server.set_http_handler(websocketpp::lib::bind(&OSCQueryServer::on_http,this,websocketpp::lib::placeholders::_1));
            }

            ~OSCQueryServer()
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

}
