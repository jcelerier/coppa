#include <coppa/oscquery/device/local.hpp>
#include <coppa/protocol/websockets/server.hpp>
#include <chrono>

// Found on stackoverflow
std::string random_string( size_t length )
{
  auto randchar = []() -> char
  {
                  const char charset[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
                  const size_t max_index = (sizeof(charset) - 1);
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
  SynchronizingLocalDevice<coppa::WebSocketServer> dev;

  // Create some parameters
  Parameter aParam;
  aParam.destination = "/da/da";
  addValue(aParam, 42, {{}, {}, {}});
  aParam.accessmode = coppa::Access::Mode::Set;
  aParam.tags = {"wow", "much tag"};

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

  std::cerr << "Size: " << dev.map().size() << std::endl;


  std::mutex test_mutex;

  // A thread that will periodically update a value.
  std::thread valueUpdateThread([&] ()
  {
    int i = 0;

    coppa::oscquery::Values v;
    v.values.push_back(0);
    while(true)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::lock_guard<std::mutex> lock(test_mutex);

      v.values[0] = i++;
      if(dev.map().has("da/do"))
        dev.update("/da/do", v);
      else
        return;
    }
  });



  // A thread that will periodically add a parameter.
  std::thread parameterAddThread([&] ()
  {
    while(true)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      std::lock_guard<std::mutex> lock(test_mutex);

      auto size = dev.map().size();
      auto randompath = dev.map()[rand() % size].destination;
      Parameter p;

      // TODO empty names should not be allowed by ParameterMap
      std::string name = random_string(1 + rand() % 2);
      if(randompath == "/")
      { p.destination = randompath + name; }
      else
      { p.destination = randompath + "/" + name; }

      dev.add(p);
    }
  });


  std::thread parameterRemoveThread([&] ()
  {
    while(true)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(400));
      std::lock_guard<std::mutex> lock(test_mutex);

      auto size = dev.map().size();
      auto randompath = dev.map()[rand() % size].destination;

      // Note : we should not be able to remove the root
      // (or if we do, it juste replaces it with an empty root).
      dev.remove(randompath);
    }
  });


  // Start the websocket server
  dev.expose();

  return 0;
}
