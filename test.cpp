#include "coppa.hpp"
#include <iostream>

/*
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

// 2 kinds of protocols :
// - query protocol (OSCQuery, Minuit)
// - data protocol (OSC, MIDI, WebSocket)
// Query protocol : can be over OSC / WebSocket / plain TCP ?
// Ex. : Minuit over OSC, Minuit over WebSockets, OSCQuery over WebSockets
// Both Minuit and OSCQuery allow for "listenability" of a value.

typedef websocketpp::server<websocketpp::config::asio> server;

void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
        std::cout << msg->get_payload() << std::endl;
}

int plop()
{
    server print_server;

    print_server.set_message_handler(&on_message);

    print_server.init_asio();
    print_server.listen(9002);
    print_server.start_accept();

    print_server.run();
}
*/
class OSCQuery
{

};




class AnAttribute
{
	coppa_name(AnAttribute);
	public:
		int a;
};

class BogusAttribute
{
	coppa_name(BogusAttribute);
	public:
		int b;
};

int main()
{
	//plop();
	using namespace eggs::variants;
	using namespace std;

	coppa::Attributes<AnAttribute, BogusAttribute> plop;
	cout << plop.m_attributes[0] << std::endl;

	coppa::ParameterAdapter<coppa::Tags, coppa::Alias> flap;
	for(auto elt : flap.attributes())
	{
		cout << elt << " ";
	}
	cout << endl;

	coppa::OSCQueryParameter flop;
	for(auto elt : flop.attributes())
	{
		cout << elt << " ";
	}

	flop.values = {{5}};
	cout << endl
		 << flop.get<coppa::RepetitionFilter>().repetitionFilter << " "
		 << get<int>(flop.values[0].value) << " "
		 << endl;

	if(flop.hasAttribute("RepetitionFilter"))
	{
		cout << "OK: " << flop.get_dyn<coppa::RepetitionFilter>()->repetitionFilter << endl;
	}

	if(flop.hasAttribute("A"))
	{
		cout << "BAD: " << flop.get_dyn<AnAttribute>()->a << endl;
	}

	coppa::OSCQueryParameter aParam;
	aParam.destination = "/da/da";
	coppa::OSCQueryParameter anotherParam;
	anotherParam.destination = "/da/do";
	anotherParam.values.push_back({5, // Value
								  {{}, {}, {4, 5, 6}}, // Range
								  coppa::ClipMode::Both}); // ClipMode
	anotherParam.values.push_back({std::string("plip")});
	anotherParam.values.push_back({3.45f,  // Value
								  {coppa::Variant(0.0f), // Range min
								   coppa::Variant(5.5f), // Range max
								   {} // Range values
								  } // Range
								  });
	anotherParam.accessMode = coppa::AccessMode::Both;

	coppa::OSCQueryParameterMap map;
	map.insert(aParam);
	map.insert(anotherParam);

	toJson(map);

	// There is no way to query anything for a remote OSC device, the tree must
	// come from the local application. Or maybe from a JSON Namespace.
	/*
	coppa::RemoteDevice<OSC> osc_dev;
	dev.addParameter(flop);

	// Here we can query the remote device.
	coppa::RemoteDevice<Minuit> minuit_dev;

	// Here the device tree is fixed.
	coppa::RemoteDevice<MIDI> midi_dev;

	// Here we can also query the remote device.
	coppa::RemoteDevice<WebSocket> ws_dev;
*/
	// Separate the transport and application layer.
	// Note : not everything can work everywhere.
	// For instance, OSCQuery and Minuit messages can be sent via OSC or websockets.

	// Here we can query bidirectionnaly the remote device.
	//coppa::RemoteDevice<OSCQuery> oscq_dev;
	// Impl can only be a copy of a remote tree (Minuit client, websocket client), or can be local and expose (Minuit, WS server).
	// -> depends on the impl.
	// Maybe both ? no.

	// Can make a tree from an impl (ex. : midi)
	// OSC : local
	return 0;
}
