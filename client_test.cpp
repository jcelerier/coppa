#include <coppa/oscquery/device/remote.hpp>


int main()
{
    using namespace std;
    using namespace coppa::oscquery;
    RemoteDevice dev("http://127.0.0.1:9002/");
    this_thread::sleep_for(chrono::milliseconds(100));
    dev.update();

    this_thread::sleep_for(chrono::seconds(1));

    cerr <<  dev.map().get<0>().size() << endl;

    // We enable listening on this address
    if(dev.has("/da/do"))
    {
        cout << "Listening on /da/do" << endl;
        dev.listenAddress("/da/do", true);
    }


    int i = 0;
    while(true)
    {
        this_thread::sleep_for(chrono::seconds(1));

        auto theMap = dev.map();
        cout << endl << "Current addresses: " << endl;

        // Print the real parameters in the tree
        for(auto&& elt : theMap.get<0>())
        {
            cout << elt.destination << endl;
        }

        // We update a value on the remote device
        Values vals;
        vals.values.push_back(i++);
        if(dev.has("/da/da"))
        {
            cout << "Setting /da/da to " << i << endl;
            dev.set("/da/da", vals);
        }
    }

    return 0;
}
