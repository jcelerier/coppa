#include <chrono>
#include <thread>
#include <coppa/protocol/osc/oscsender.hpp>
#include <coppa/tools/random.hpp>

int main(int argc, char** argv)
{
  OscSender s("127.0.0.1", 1234);

  while(true)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    s.send("/label/value", rand() % 15);

    s.send("/laPampa/mammamia/property/text", osc::Symbol(my_rand<std::string>().c_str()));
  }
  return 0;
}
