#pragma once
#include <coppa/coppa.hpp>
#include <jeayeson/jeayeson.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

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



using OSCQueryParameter = ParameterAdapter<Values,
										   Description,
										   Tags,
										   Alias,
										   RepetitionFilter,
										   Bounds<Variant,
											  StandardComparator<Variant>,
											  StandardComparator<Variant>>>;

using OSCQueryParameterMap = ParameterMapType<OSCQueryParameter>;

// A ParameterMap can be JSON'd
json_map toJson(const OSCQueryParameterMap& map)
{
	using namespace std;
	using namespace boost;
	using namespace eggs::variants;
	// Root node
	json_map root;
	root["description"] = string("root node");
	root["full_path"] = string("/");
	root["access"] = 0;

	// Create a tree with the parameters
	for(const auto& parameter : map)
	{
		char_separator<char> sep("/");
		tokenizer<char_separator<char>> tokens(parameter.destination, sep);

		// Create the required parts of the tree and navigate to the corresponding node
		auto* current_map = &root;
		for(const auto& token : tokens)
		{
			if(!current_map->has("contents"))
			{ current_map->set("contents", json_map{}); }

			current_map = &current_map->get_for_path<json_map>("contents");

			if(!current_map->has(token))
			{ current_map->set(token, json_map{}); }

			current_map = &current_map->get_for_path<json_map>(token);
		}

		// These attributes are always here
		current_map->set("full_path", parameter.destination);
		current_map->set("access", static_cast<int>(parameter.accessMode));

		// Potentially empty attributes :
		if(!parameter.description.empty())
		{ current_map->set("description", parameter.description); }

		if(!parameter.tags.empty())
		{
			json_array arr;
			for(const auto& tag : parameter.tags)
				arr.add(tag);

			current_map->set("tags", arr);
		}

		// Handling of the types / values
		if(!parameter.values.empty())
		{
			std::string str_type;
			json_array value_arr;
			json_array clip_arr;
			json_array range_arr;
			for(const auto& oscqvalue : parameter.values)
			{
				const auto& value = oscqvalue.value;
				switch(value.which())
				{
					case 0: // int
						str_type += "i";
						value_arr.add(get<int>(value));
						break;
					case 1: // float
						str_type += "f";
						value_arr.add(get<float>(value));
						break;
					case 2: // bool : no bool in OSCQuery ?
						//str_type += "b";
						//value_arr.add(get<bool>(value));
						break;
					case 3: // string
						str_type += "s";
						value_arr.add(get<std::string>(value));
						break;
					case 4: // generic
						str_type += "b";
						value_arr.add(get<const char*>(value));
						break;
				}

				switch(oscqvalue.clipMode)
				{
					case ClipMode::None: clip_arr.add("None"); break;
					case ClipMode::Low:  clip_arr.add("Low");  break;
					case ClipMode::High: clip_arr.add("High"); break;
					case ClipMode::Both: clip_arr.add("Both"); break;
				}

				json_array range_subarray;
				if(!oscqvalue.range.min)
				{
					range_subarray.add("null");
				}
				else
				{
					const auto& minValue = *oscqvalue.range.min;
					switch(minValue.which())
					{
						case 0: range_subarray.add(get<int>(minValue)); break;
						case 1: range_subarray.add(get<float>(minValue)); break;
						case 2: // bool : no bool in OSCQuery ?
							//value_arr.add(get<bool>(minValue));
							break;
						case 3: range_subarray.add(get<string>(minValue)); break;
						case 4: range_subarray.add(get<const char*>(minValue)); break;
					}
				}

				if(!oscqvalue.range.max)
				{
					range_subarray.add("null");
				}
				else
				{
					const auto& maxValue = *oscqvalue.range.max;
					switch(maxValue.which())
					{
						case 0: range_subarray.add(get<int>(maxValue)); break;
						case 1: range_subarray.add(get<float>(maxValue)); break;
						case 2: // bool : no bool in OSCQuery ?
							//value_arr.add(get<bool>(maxValue));
							break;
						case 3: range_subarray.add(get<string>(maxValue)); break;
						case 4: range_subarray.add(get<const char*>(maxValue)); break;
					}
				}

				if(oscqvalue.range.values.empty())
				{
					range_subarray.add("null");
				}
				else
				{
					json_array range_values_array;
					for(auto& elt : oscqvalue.range.values)
					{
						switch(elt.which())
						{
							case 0: range_values_array.add(get<int>(elt)); break;
							case 1: range_values_array.add(get<float>(elt)); break;
							case 2: // bool : no bool in OSCQuery ?
								//value_arr.add(get<bool>(elt));
								break;
							case 3: range_values_array.add(get<string>(elt)); break;
							case 4: range_values_array.add(get<const char*>(elt)); break;
						}
					}
					range_subarray.add(range_values_array);
				}

				range_arr.add(range_subarray);
			}

			current_map->set("type", str_type);
			current_map->set("value", value_arr);
			current_map->set("clip_mode", clip_arr);
			current_map->set("range_mode", range_arr);
		}
	}

	// 2. For each parameter set its attributes
	std::cout << root;
	return root;
}



class OSCQueryServer
{
	OSCQueryParameterMap m_map;
	public:

	using server = websocketpp::server<websocketpp::config::asio>;
	    void on_http(websocketpp::connection_hdl hdl)
	    {
		    server::connection_ptr con = m_server.get_con_from_hdl(hdl);

		    // Set status to 200 rather than the default error code
		    con->set_status(websocketpp::http::status_code::ok);
		    // Set body text to the HTML created above
		    con->set_body(toJson(m_map).to_string());
		}

	    OSCQueryServer(OSCQueryParameterMap map):
			m_map{map}
	    {
	        m_server.init_asio();

	        m_server.set_open_handler(websocketpp::lib::bind(&OSCQueryServer::on_open,this,websocketpp::lib::placeholders::_1));
	        m_server.set_close_handler(websocketpp::lib::bind(&OSCQueryServer::on_close,this,websocketpp::lib::placeholders::_1));
	        m_server.set_message_handler(websocketpp::lib::bind(&OSCQueryServer::on_message,this,websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
	        m_server.set_http_handler(websocketpp::lib::bind(&OSCQueryServer::on_http,this,websocketpp::lib::placeholders::_1));
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

/*
class OSCQueryServer
{
	using server = websocketpp::server<websocketpp::config::asio>;

	public:
		void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
		{
			std::cout << msg->get_payload() << std::endl;
		}

		void serve()
		{
		    server print_server;

		    print_server.set_message_handler(std::bind(&OSCQueryServer::on_message,
													   this,
													   std::placeholders::_1,
													   std::placeholders::_2));

		    print_server.init_asio();
		    print_server.listen(9002);
		    print_server.start_accept();

		    print_server.run();
		}

};*/
}
