#pragma once
#include <coppa/oscquery/map.hpp>
#include <random>

// Found on stackoverflow
inline std::string random_string( size_t length )
{
  auto randchar = []() -> char
  {
                  static const char charset[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
                  const size_t max_index = (sizeof(charset) - 1);
                  return charset[ rand() % max_index ]; };
std::string str(length,0);
std::generate_n( str.begin(), length, randchar );
return str;
}

template<typename T>
T my_rand();

template<>
inline int my_rand<int>()
{
  using namespace std;
  static random_device rd;
  static mt19937 gen(rd());
  static uniform_int_distribution<int>
      dist(numeric_limits<int>::min(),
           numeric_limits<int>::max());

  return dist(gen);
}

template<>
inline float my_rand<float>()
{
  using namespace std;
  static random_device rd;
  static mt19937 gen(rd());
  static uniform_real_distribution<float>
      dist(numeric_limits<float>::min(),
           numeric_limits<float>::max());

  return dist(gen);
}


template<>
inline std::string my_rand<std::string>()
{
  using namespace std;
  static random_device rd;
  static mt19937 gen(rd());
  static uniform_int_distribution<int>
      dist(1,
           15);

  return random_string(dist(gen));
}


template<typename T>
inline coppa::oscquery::Range random_range()
{
  using namespace std;
  coppa::oscquery::Range r;
  // no vals
  if(my_rand<int>() % 2)
  {
    if(my_rand<int>() % 2) r.min = my_rand<T>();
    if(my_rand<int>() % 2) r.max = my_rand<T>();
  }
  else // vals
  {
    int n = my_rand<int>() % 50;
    for(int i = 0; i < n; i++)
    {
      r.values.push_back(my_rand<T>());
    }
  }

  return r;
}

inline coppa::oscquery::Parameter random_anonymous_parameter()
{
  using namespace coppa;
  using namespace coppa::oscquery;
  Parameter param;

  int nvalues = my_rand<int>() % 10;
  for(int i = 0; i < nvalues; i++)
  { // int, float, string
    switch(my_rand<int>() % 3)
    {
      case 0: // int
        addValue(param, my_rand<int>(), random_range<int>(), static_cast<ClipMode>(my_rand<int>() % 4));
        break;
      case 1: // float
        addValue(param, my_rand<float>(), random_range<float>(), static_cast<ClipMode>(my_rand<int>() % 4));
        break;
      case 2: // string
        addValue(param, my_rand<std::string>(), random_range<std::string>(), static_cast<ClipMode>(my_rand<int>() % 4));
        break;
    }
  }

  param.description = my_rand<std::string>();
  param.accessmode = static_cast<Access::Mode>(my_rand<int>() % 4);

  int ntags = my_rand<int>() % 10;
  for(int i = 0; i < ntags; i++)
  {
    param.tags.push_back(my_rand<std::string>());
  }

  return param;
}


inline auto random_map()
{
  using namespace coppa;
  using namespace coppa::oscquery;
  basic_map<coppa::oscquery::ParameterMap> map;

  auto maxsize = my_rand<int>() % 10000 + 1;
  for(int i = 0; i < maxsize; i++)
  {
    auto randompath = map[my_rand<int>() % map.size()].destination;
    Parameter p = random_anonymous_parameter();

    // TODO empty names should not be allowed by ParameterMap
    std::string name = my_rand<std::string>();
    if(randompath == "/")
    { p.destination = randompath + name; }
    else
    { p.destination = randompath + "/" + name; }

    map.insert(p);
  }

  return map;
}

template<typename Map>
inline auto setup_basic_map(Map& map)
{
  using namespace coppa::oscquery;

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
  map.insert(aParam);
  map.insert(bParam);
  map.insert(cParam);
  map.insert(anotherParam);

}
