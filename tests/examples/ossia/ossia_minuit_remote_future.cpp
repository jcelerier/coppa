#include <coppa/minuit/device/minuit_remote_future.hpp>

#include <thread>
using namespace coppa;
using namespace coppa::ossia;


int main()
{
  // TODO keep working on the idea of a compile-time definable parameter which ends on a map<variant>
  basic_map<ParameterMapType<Parameter>> base_map;
  locked_map<basic_map<ParameterMapType<Parameter>>> map(base_map);

  // Set-up our device
  minuit_remote_impl_future remote("rimoute", map, 13579, "127.0.0.1", 9998);

  // Send a refresh request
  auto fut_refresh = remote.refresh();

  // Wait until we get a full namespace
  fut_refresh.wait();

  // Show our namespace
  for(auto elt : remote.map())
  {
    std::cerr << elt.destination << std::endl;
  }

  // Now try to get a value
  auto get_promise = remote.pull("/tutu");
  get_promise.wait();
  auto res = get_promise.get();

  std::cerr << "got a result" << res.destination << res.variants.size();


}
