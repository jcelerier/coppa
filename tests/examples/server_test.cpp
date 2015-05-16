#include <coppa/oscquery/device/local.hpp>
#include <coppa/protocol/websockets/server.hpp>
#include <coppa/tools/random.hpp>
#include <chrono>
int main()
{
  using namespace coppa::oscquery;

  // Create a device
  SynchronizingLocalDevice<coppa::WebSocketServer> dev;
  setup_basic_map(dev.map());

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
      auto randompath = dev.map()[my_rand<int>() % size].destination;
      Parameter p;

      // TODO empty names should not be allowed by ParameterMap
      std::string name = my_rand<std::string>();
      if(randompath == "/")
      { p.destination = randompath + name; }
      else
      { p.destination = randompath + "/" + name; }

      dev.add(p);
    }
  });

  /*
  std::thread parameterRemoveThread([&] ()
  {
    while(true)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(400));
      std::lock_guard<std::mutex> lock(test_mutex);

      auto size = dev.map().size();
      auto randompath = dev.map()[rand2() % size].destination;

      // Note : we should not be able to remove the root
      // (or if we do, it juste replaces it with an empty root).
      dev.remove(randompath);
    }
  });
*/

  // Start the websocket server
  dev.expose();

  return 0;
}
