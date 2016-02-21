#include <coppa/minuit/device/local.hpp>

using namespace coppa;
using namespace coppa::ossia;
auto make_parameter(std::string name)
{
  Parameter p;
  p.destination = name;
  return p;
}

int main()
{
  
  basic_map<ParameterMapType<Parameter>> base_map;
  
  base_map.insert(make_parameter("/titi"));
  base_map.insert(make_parameter("/tutu"));
  base_map.insert(make_parameter("/titi/plop"));
  locked_map<basic_map<ParameterMapType<Parameter>>> map(base_map);
  
  minuit_local_impl test(map, 9998, "127.0.0.1", 13579);
  
  auto test_functor = [] (const Parameter& p) {
    std::cerr << p.destination << std::endl;
  };

  test.on_value_changed.connect(test_functor);
  
  while(true)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}
