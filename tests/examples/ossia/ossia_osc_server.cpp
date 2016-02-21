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

  auto p1 = make_parameter("/titi");
  p1.variants.push_back(3);
  p1.variants.push_back(4);
  p1.variants.push_back(1);

  auto p2 = make_parameter("/tutu");
  p2.variants.push_back(0.5f);
  auto p3 = make_parameter("/titi/plop");
  p3.variants.push_back("fuu");
  base_map.insert(p1);
  base_map.insert(p2);
  base_map.insert(p3);
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
