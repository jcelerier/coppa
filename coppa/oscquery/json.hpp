#pragma once
#include <jeayeson/jeayeson.hpp>
#include <coppa/oscquery/parameter.hpp>
namespace coppa
{
    namespace oscquery
    {
        class JSONRead
        {
            public:
            static void readObject(const json_map obj, ParameterMap& map)
            {
                std::cout << obj.to_string() << std::endl << std::endl;
                if(obj.find("full_path") != obj.end())
                { // It's a real parameter
                    Parameter p;
                    p.destination = obj.get<std::string>("full_path");
                    if(obj.find("description") != obj.end())
                    {
                        std::cout << "Description" << std::endl;
                        p.description = obj.get<std::string>("description");
                    }

                    if(obj.find("tags") != obj.end())
                    {
                        std::cout << "Tags" << std::endl;
                        json_array json_tags = obj.get<json_array>("tags");
                        for(auto&& elt : json_tags)
                            p.tags.push_back(elt.get<std::string>());
                    }
                    if(obj.find("access") != obj.end())
                    {
                        std::cout << "Access" << std::endl;
                       p.accessmode = static_cast<AccessMode>(obj.get<int>("access"));
                    }

                    if(obj.find("value") != obj.end())
                    {
                        std::cout << "Value" << std::endl;
                        json_array json_vals = obj.get<json_array>("value");
                        for(json_value val : json_vals)
                        {
                            switch(val.get_type())
                            {
                                case json_value::type::integer:
                                    p.values.push_back(int(val.get<int>()));
                                    break;
                                case json_value::type::real:
                                    p.values.push_back(float(val.get<double>()));
                                    break;
                                case json_value::type::boolean:
                                    //p.values.values.push_back(int(val.get<int>()));
                                    break;
                                case json_value::type::string:
                                    p.values.push_back(val.get<std::string>());
                                    break;
                                // TODO blob
                            }
                        }


                        std::cout << "Range" << std::endl;
                        // If there is value, there is also range and clipmode
                        json_array json_ranges = obj.get<json_array>("range");
                        for(json_value range_val : json_ranges)
                        {
                            Range range;
                            json_array range_arr = range_val.as<json_array>();
                            // 1 sub-array per value
                            int i = 0;
                            for (auto&& it = range_arr.begin(); it != range_arr.end(); it++, i++)
                            {
                                json_value val = *it;
                                switch(i)
                                {
                                    case 0:
                                    {
                                        std::cout << 0 << " ";
                                        switch(val.get_type())
                                        {
                                            case json_value::type::integer:
                                                range.min = Variant(int(val.get<int>()));
                                                break;
                                            case json_value::type::real:
                                                range.min = Variant(float(val.get<double>()));
                                                break;
                                            case json_value::type::boolean:
                                                break;
                                            case json_value::type::string:
                                                range.min = Variant(val.get<std::string>());
                                                break;
                                            // TODO blob
                                        }
                                        break;
                                    }
                                    case 1:
                                    {
                                        std::cout << 1 << " ";
                                        switch(val.get_type())
                                        {
                                            case json_value::type::integer:
                                                range.max = Variant(int(val.get<int>()));
                                                break;
                                            case json_value::type::real:
                                                range.max = Variant(float(val.get<double>()));
                                                break;
                                            case json_value::type::boolean:
                                                break;
                                            case json_value::type::string:
                                                range.max = Variant(val.get<std::string>());
                                                break;
                                            // TODO blob
                                        }
                                        break;
                                    }
                                    case 2:
                                    {
                                        std::cout << 2 << " ";
                                        if(val.get_type() == json_value::type::array)
                                        {
                                            auto enum_arr = val.as<json_array>();
                                            for(json_value enum_val : enum_arr)
                                            {
                                                switch(enum_val.get_type())
                                                {
                                                    case json_value::type::integer:
                                                        range.values.push_back(int(enum_val.get<int>()));
                                                        break;
                                                    case json_value::type::real:
                                                        range.values.push_back(float(enum_val.get<float>()));
                                                        break;
                                                    case json_value::type::boolean:
                                                        //range.values.push_back(int(val.get<int>()));
                                                        break;
                                                    case json_value::type::string:
                                                        range.values.push_back(enum_val.get<std::string>());
                                                        break;
                                                        // TODO blob
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            p.ranges.push_back(range);
                        }

                        // Clipmode

                        std::cout << "Clipmode" << std::endl;
                        json_array json_clipmodes = obj.get<json_array>("clipmode");
                        for(json_value value : json_clipmodes)
                        {
                            std::string text = value.get<std::string>();
                            if(text == "None")
                            {
                                p.clipmodes.push_back(ClipMode::None);
                            }
                            else if(text == "Low")
                            {
                                p.clipmodes.push_back(ClipMode::Low);
                            }
                            else if(text == "High")
                            {
                                p.clipmodes.push_back(ClipMode::High);
                            }
                            else if(text == "Both")
                            {
                                p.clipmodes.push_back(ClipMode::Both);
                            }
                        }
                    }

                    map.insert(p);
                }

                // Recurse on the children
                if(obj.find("contents") != obj.end())
                {
                    // contents is a json_map where each child is a key / json_map
                    json_map contents = obj.get<json_map>("contents");
                    for(auto key : contents.get_keys())
                    {
                        readObject(contents.get<json_map>(key), map);
                    }
                }
            }

            static ParameterMap toMap(const std::string& message)
            {
                json_map obj{message};
                ParameterMap map;

                readObject(obj, map);

                return map;
            }
        };

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

                static std::string insertParameterMessage(const Parameter&)
                {
                    return {};
                }

                static std::string removePathMessage(const std::string& )
                {
                    return {};
                }

                static std::string deviceInfo(int port)
                {
                    json_map map;
                    map["osc_port"] = port;

                    return map.to_string();
                }

                template<typename Attribute>
                static std::string attributeChangedMessage(const std::string& path, const Attribute& attr)
                {
                    // TODO what if type changed?
                    json_map map;
                    map["path_changed"] = path;
                    map[attributeToKey(attr)] = attributeToJson(attr);
                    return map.to_string();
                }

            private:
                static constexpr const char* attributeToKey(const Values& ) { return "value"; }
                static constexpr const char* attributeToKey(const Ranges& ) { return "range"; }
                static constexpr const char* attributeToKey(const ClipModes& ) { return "clipmode"; }
                static constexpr const char* attributeToKey(const Access& ) { return "access"; }
                static constexpr const char* attributeToKey(const Description& ) { return "description"; }
                static constexpr const char* attributeToKey(const Tags& ) { return "tags"; }

                static auto attributeToJson(const Values& val) { return getJsonValueArray(val); }
                static auto attributeToJson(const Ranges& val) { return getJsonRangeArray(val); }
                static auto attributeToJson(const ClipModes& val) { return getJsonClipModeArray(val); }
                static auto attributeToJson(const Access& val) { return static_cast<int>(val.accessmode); }
                static auto attributeToJson(const Description& val) { return val.description; }
                static auto attributeToJson(const Tags& val) { return getJsonTags(val); }

                static std::string getJsonTypeString(const Parameter& parameter)
                {
                    std::string str_type;
                    for(const auto& value : parameter.values)
                    {
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

                static json_array getJsonValueArray(const Values& values)
                {
                    json_array value_arr;
                    for(const auto& value : values.values)
                    {
                        addValueToJsonArray(value_arr, value);
                    }

                    return value_arr;
                }

                static json_array getJsonClipModeArray(const ClipModes& clipmodes)
                {
                    json_array clip_arr;
                    for(const auto& clipmode : clipmodes.clipmodes)
                    {
                        switch(clipmode)
                        {
                            case ClipMode::None: clip_arr.add("None"); break;
                            case ClipMode::Low:  clip_arr.add("Low");  break;
                            case ClipMode::High: clip_arr.add("High"); break;
                            case ClipMode::Both: clip_arr.add("Both"); break;
                        }
                    }

                    return clip_arr;
                }

                static json_array getJsonRangeArray(const Ranges& ranges)
                {
                    json_array range_arr;
                    for(const auto& range : ranges.ranges)
                    {
                        json_array range_subarray;
                        if(!range.min)
                        { range_subarray.add(json_value::null_t{}); }
                        else
                        { addValueToJsonArray(range_subarray, *range.min); }

                        if(!range.max)
                        { range_subarray.add(json_value::null_t{}); }
                        else
                        { addValueToJsonArray(range_subarray, *range.max); }

                        if(range.values.empty())
                        { range_subarray.add(json_value::null_t{}); }
                        else
                        {
                            json_array range_values_array;
                            for(auto& elt : range.values)
                            {
                                addValueToJsonArray(range_values_array, elt);
                            }
                            range_subarray.add(range_values_array);
                        }

                        range_arr.add(range_subarray);
                    }

                    return range_arr;
                }

                static json_array getJsonTags(const Tags& tags)
                {
                    json_array arr;
                    for(const auto& tag : tags.tags)
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
                    { map.set("access", static_cast<int>(parameter.accessmode)); }
                    else if(method == "type")
                    { map.set("type", getJsonTypeString(parameter)); }
                    else if(method == "description")
                    { map.set("description", parameter.description); }
                    else if(method == "tags")
                    { map.set("tags", getJsonTags(parameter)); }
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
                    obj.set("access", static_cast<int>(parameter.accessmode));

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
    }
}
