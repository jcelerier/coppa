#include <coppa/minuit/device/local.hpp>
#include <coppa/minuit/device/minuit_remote_behaviour.hpp>
#include <thread>
using namespace coppa;
using namespace coppa::ossia;

class minuit_remote_impl : public osc_local_device<
    locked_map<basic_map<ParameterMapType<Parameter>>>,
    osc::receiver,
    minuit_message_handler<minuit_remote_behaviour>,
    osc::sender>
{
    using parent_t::osc_local_device;
};

auto make_parameter(std::string name)
{
  Parameter p;
  p.destination = name;

  return p;
}

void refresh(minuit_remote_impl& remote)
{
  remote.map().clear();

  // first request namespace
  const char * root = "/";
  remote.sender.send(remote.name() + "?namespace", root);
}

int main()
{
  basic_map<ParameterMapType<Parameter>> base_map;
  locked_map<basic_map<ParameterMapType<Parameter>>> map(base_map);

  minuit_remote_impl remote(map, 13579, "127.0.0.1", 9998);
  refresh(remote);

  auto test_functor = [] (const Parameter& p) {
    std::cerr << p.destination << std::endl;
  };

  remote.on_value_changed.connect(test_functor);

  std::this_thread::sleep_for(std::chrono::seconds(1));
  const char * addr = "/tutu";
  auto sv = string_view(addr);
  auto pattern = remote.name() + "?get";
  auto sp = string_view(pattern);
  auto t1 = std::chrono::high_resolution_clock::now();
  for(int i = 0; i < 10000000; i++)
  {
    remote.sender.send(sv, i);
  }

  auto t2 = std::chrono::high_resolution_clock::now();
  std::cerr << "time " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << "\n";
}
