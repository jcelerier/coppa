#include <coppa/coppa.hpp>
#include <coppa/oscquery/device.hpp>


#include <iostream>


struct AnAttribute
{
        coppa_name(AnAttribute);
        int a;
};

struct BogusAttribute
{
        coppa_name(BogusAttribute);
        int b;
};

void simpleTest()
{
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

    coppa::oscquery::Parameter flop;
    for(auto elt : flop.attributes())
    {
        cout << elt << " ";
    }

    flop.values = {{5}};
    flop.description = "coucou";
    cout << endl
         << flop.get<coppa::Description>().description << " "
         << get<int>(flop.values[0].value) << " "
         << endl;

    if(flop.hasAttribute("Description"))
    {
        cout << "OK: " << flop.get_dyn<coppa::Description>()->description << endl;
    }

    if(flop.hasAttribute("A"))
    {
        cout << "BAD: " << flop.get_dyn<AnAttribute>()->a << endl;
    }

    coppa::oscquery::Parameter aParam;
    aParam.destination = "/da/da";
    coppa::oscquery::Parameter anotherParam;
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

    coppa::oscquery::ParameterMap map;
    map.insert(aParam);
    map.insert(anotherParam);


}

void oldTest()
{
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
}

void deviceTest()
{
    using namespace coppa::oscquery;

    LocalDevice<WebSocketServer> dev;
    Parameter aParam;
    aParam.destination = "/da/da";

    Parameter bParam;
    bParam.destination = "/plop/plip/plap";
    bParam.values.push_back({std::string("Why yes young chap"),
                             {{}, {}, {}},
                             coppa::ClipMode::None});

    Parameter cParam;
    cParam.destination = "/plop";

    Parameter anotherParam;
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

    dev.add(aParam);
    dev.add(bParam);
    dev.add(cParam);
    dev.add(anotherParam);

    dev.expose();
}

void parameterTest()
{
    /*
    coppa::oscquery::Parameter param;
    param.Description::set("dada");
    param.get<coppa::Description>().get();
    */
}

int main()
{
    parameterTest();

    return 0;
}
