#include <coppa/minuit/device/minuit_listening_local.hpp>


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
  auto p2 = make_parameter("/tutu");
  auto p3 = make_parameter("/titi/plop");
  auto p4 = make_parameter("/foo/bar/baz");
  auto p5 = make_parameter("/foo/blah");

  p2.variants.push_back(0.5f);
  p2.min = -25.f;
  p2.max = 25.f;

  p3.variants.push_back(std::string("fuu"));

  p4.variants.push_back(true);

  p5.variants.push_back(3);
  p5.variants.push_back(4);
  p5.variants.push_back(1);
  p5.access = Access::Mode::Get;

  base_map.insert(p1);
  base_map.insert(p2);
  base_map.insert(p3);
  base_map.insert(p4);
  base_map.insert(p5);
  locked_map<basic_map<ParameterMapType<Parameter>>> map(base_map);

  minuit_listening_local_device test(map, 9998, "127.0.0.1", 13579);

  auto test_functor = [] (const Parameter& p) {
    std::cerr << p.destination << std::endl;
  };

  test.on_value_changed.connect(test_functor);

  while(true)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    test.update<string_view>(p2.destination, [] (auto& param) {
      param.variants[0] = float(rand());
    });
  }
}
