#include <coppa/oscquery/device/local.hpp>
#include <coppa/protocol/websockets/server.hpp>

int main()
{
    using namespace coppa::oscquery;

    // Create a device
    LocalDevice<coppa::WebSocketServer> dev;

    // Create some parameters
    Parameter aParam;
    aParam.destination = "/da/da";
    addValue(aParam, 42, {{}, {}, {}});
    aParam.accessmode = coppa::Access::Mode::Set;
    aParam.tags = {{"wow", "much tag"}};

    Parameter bParam;
    bParam.destination = "/plop/plip/plap";
    addValue(bParam,
             std::string("Why yes young chap"),
             {{}, {}, {}},
             coppa::ClipMode::None);

    Parameter cParam;
    cParam.destination = "/plop";
    cParam.description = "A quite interesting parameter";

    Parameter anotherParam;
    anotherParam.destination = "/da/do";
    anotherParam.accessmode = coppa::Access::Mode::Both;
    addValue(anotherParam, 5, // Value
                           {{}, {}, {4, 5, 6}}, // Range
                           coppa::ClipMode::Both); // ClipMode

    addValue(anotherParam, std::string("plip"));
    addValue(anotherParam, 3.45f,  // Value
                            {coppa::Variant(0.0f), // Range min
                             coppa::Variant(5.5f), // Range max
                             {} // Range values
                            });

    // Add them to the device
    dev.add(aParam);
    dev.add(bParam);
    dev.add(cParam);
    dev.add(anotherParam);

    // A thread that will periodically update a value.
    std::thread aThread([&] ()
    {
        int i = 0;

        coppa::oscquery::Values v;
        v.values.push_back(0);
        while(true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            v.values[0] = i++;
            dev.update("/da/do", v);
        }
    });


    // Start the websocket server
    dev.expose();

    return 0;
}
