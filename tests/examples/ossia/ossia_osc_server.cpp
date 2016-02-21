#include <coppa/minuit/device/local.hpp>

int main()
{
  using namespace coppa;
  using namespace coppa::ossia;
  
  basic_map<ParameterMapType<Parameter>> base_map;
  locked_map<basic_map<ParameterMapType<Parameter>>> map(base_map);
  
  minuit_local_impl test(map, 1234, "127.0.0.1", 13579);
  
  auto test_functor = [] (const Parameter& p) {
    std::cerr << p.destination << std::endl;
  };

  test.on_value_changed.connect(test_functor);
  
  while(true)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}
