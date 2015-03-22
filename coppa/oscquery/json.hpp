#pragma once
#include <jeayeson/jeayeson.hpp>

namespace coppa
{
    namespace oscquery
    {
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
    }
}
