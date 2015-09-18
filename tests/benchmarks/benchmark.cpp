#include <coppa/device/local.hpp>
#include <coppa/oscquery/device/local.hpp>
#include <coppa/protocol/websockets/server.hpp>
#include <chrono>

// Found on stackoverflow
std::string random_string( size_t length )
{
    auto randchar = []() -> char
    {
        static const constexpr char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        static const constexpr size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, randchar );
    return str;
}

int main()
{
    using namespace coppa::oscquery;

    // Create a device
    local_device dev;

    // A thread that will periodically add a parameter.
    std::thread parameterAddThread([&] ()
    {
      while(true)
      {
        auto gstart = std::chrono::steady_clock::now();
        auto randompath = dev[rand() % dev.size()].destination;
        auto gend = std::chrono::steady_clock::now();
        auto gdiff = gend - gstart;
        std::cout << "Getting: " << dev.size() << "  " << std::chrono::duration <double, std::milli> (gdiff).count() << "" << std::endl;

        Parameter p;
        // TODO empty names should not be allowed by ParameterMap
        std::string name = random_string(1 + rand() % 2);
        if(randompath == "/")
        {
          p.destination = randompath + name;
        }
        else
        {
          p.destination = randompath + "/" + name;
        }

        auto start = std::chrono::steady_clock::now();
        dev.insert(p);
        auto end = std::chrono::steady_clock::now();
        auto diff = end - start;
        std::cout << "Adding: " << dev.size() << "  " << std::chrono::duration <double, std::milli> (diff).count() << "" << std::endl;
      }
    });


    // Start the websocket server
    dev.expose();

    return 0;
}
