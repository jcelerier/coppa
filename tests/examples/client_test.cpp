#include <coppa/oscquery/device/remote.hpp>


int main()
{
    using namespace std;
    using namespace coppa::oscquery;
    remote_device dev("http://127.0.0.1:9002/");
    dev.queryConnectAsync();
    while(!dev.queryConnected())
      this_thread::sleep_for(chrono::milliseconds(100));

    dev.queryNamespace();

    this_thread::sleep_for(chrono::seconds(1));

    cerr <<  dev.safeMap().size() << endl;

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

        const auto& theMap = dev.safeMap();
        cout << endl << "Current addresses: " << endl;

        // Print the real parameters in the tree
        for(auto&& elt : theMap.unsafeMap())
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
