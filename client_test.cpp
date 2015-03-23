#include <coppa/oscquery/oscquery.hpp>


int main()
{
    using namespace coppa::oscquery;
    RemoteDevice dev("http://127.0.0.1:9002/");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    dev.update();

    //std::this_thread::sleep_for(std::chrono::seconds(1));

    dev.listenAddress("/da/do", true);

    while(true)
    {
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        for(auto& elt : dev.map().get<0>())
        {
            std::cout << elt.destination << std::endl;
        }
        if(dev.has("/da/da"))
            dev.set("/da/da", Values{{1}});
    }

    return 0;
}
