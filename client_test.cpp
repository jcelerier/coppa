#include <coppa/oscquery/oscquery.hpp>


int main()
{
    using namespace coppa::oscquery;
    RemoteDevice dev("http://127.0.0.1:9002/");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    dev.update();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cerr <<  dev.map().get<0>().size() << std::endl;

    // We enable listening on this address
    if(dev.has("/da/do"))
    {
        std::cout << "Listening on /da/do" << std::endl;
        dev.listenAddress("/da/do", true);
    }


    int i = 0;
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto theMap = dev.map();
        std::cout << "\n\nCurrent addresses: " << std::endl;
        for(auto&& elt : theMap.get<0>())
        {
            std::cout << elt.destination << std::endl;
        }

        Values vals;
        vals.values.push_back(i++);
        if(dev.has("/da/da"))
        {
            std::cerr << "Setting /da/da to " << i  << std::endl;
            dev.set("/da/da", vals);
        }
    }

    return 0;
}
