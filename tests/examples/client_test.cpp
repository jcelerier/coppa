#include <coppa/oscquery/device/remote.hpp>

int main()
{
    using namespace std;
    using namespace coppa::oscquery;
    coppa::basic_map<coppa::oscquery::ParameterMap> base_map;
    coppa::locked_map<coppa::basic_map<coppa::oscquery::ParameterMap>> locked_map(base_map);    
    coppa::remote_map_setter<
        coppa::locked_map<coppa::basic_map<coppa::oscquery::ParameterMap>>, 
        coppa::osc::sender> map_setter(locked_map);
    remote_device dev(map_setter, "http://127.0.0.1:9002/");
    dev.query_connect_async();
    while(!dev.query_is_connected())
      this_thread::sleep_for(chrono::milliseconds(100));

    dev.query_request_namespace();

    this_thread::sleep_for(chrono::seconds(1));

    cerr << dev.map().size() << endl;

    // We enable listening on this address
    if(dev.map().has("/da/do"))
    {
        cout << "Listening on /da/do" << endl;
        dev.query_listen_address("/da/do", true);
    }


    int i = 0;
    while(true)
    {
        this_thread::sleep_for(chrono::seconds(1));

        const auto& theMap = dev.map();
        cout << endl << "Current addresses: " << endl;

        // Print the real parameters in the tree
        for(auto&& elt : theMap.get_data_map())
        {
            cout << elt.destination << endl;
        }

        // We update a value on the remote device
        Values vals;
        vals.values.push_back(i++);
        if(dev.map().has("/da/da"))
        {
            cout << "Setting /da/da to " << i << endl;
            map_setter.set("/da/da", vals);
        }
    }

    return 0;
}
