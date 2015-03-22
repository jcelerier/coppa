#include <coppa/coppa.oscquery.hpp>


int main()
{
    using namespace coppa::oscquery;
    RemoteDevice dev("http://127.0.0.1:9002/");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    dev.update();

    while(true)
    { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }

    return 0;
}
