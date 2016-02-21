#include <coppa/minuit/device/local.hpp>
#include <coppa/minuit/device/minuit_remote_behaviour.hpp>

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
  remote.sender.send(remote.name() + "?namespace", "/");
}

int main()
{
  basic_map<ParameterMapType<Parameter>> base_map;
  locked_map<basic_map<ParameterMapType<Parameter>>> map(base_map);

  minuit_remote_impl test(map, 13579, "127.0.0.1", 9998);
  refresh(test);

  auto test_functor = [] (const Parameter& p) {
    std::cerr << p.destination << std::endl;
  };

  test.on_value_changed.connect(test_functor);

  while(true)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}
