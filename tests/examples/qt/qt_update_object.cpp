#include <chrono>
#include <thread>
#include <coppa/protocol/osc/oscsender.hpp>
#include <coppa/tools/random.hpp>

#include <QDebug>
int main(int argc, char** argv)
{
  coppa::osc::sender s("127.0.0.1", 1234);

  float angle = 0;
  while(true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //s.send("/label/value", rand() % 15);

    //s.send("/laPampa/mammamia/property/text", osc::Symbol(my_rand<std::string>().c_str()));
    if(rand() % 2) angle = angle + my_rand<float>() / std::numeric_limits<float>::max() * 3;
    else angle = angle - my_rand<float>() / std::numeric_limits<float>::max() * 3;
    s.send("/GraphicsSquare/property/rotation", angle);
  }
  return 0;
}
