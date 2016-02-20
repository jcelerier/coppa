#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <coppa/minuit/parameter.hpp>
#include <coppa/minuit/device/message_handler.hpp>
#include <coppa/minuit/device/local.hpp>
#include <coppa/tools/random.hpp>
using namespace coppa;
using namespace coppa::ossia;
using namespace eggs::variants;
using namespace std;




TEST_CASE( "value init", "[ossia][value]" ) {
    coppa::ossia::Variant v;
    Tuple vals_in;
    vals_in.variants = {1, 2, 3};
    v = vals_in;
    auto vals_out = get<Tuple>(v);

    REQUIRE(vals_out == vals_in);
}

TEST_CASE( "parameter init", "[ossia][parameter]" ) {
    coppa::ossia::Parameter p;
}

struct mock_device
{
    Parameter p;
    Variant v;
    
    bool acquire_read_lock() const { return true; }
    template<typename T>
    auto find(T) { return &p; }
    Parameter* end() { return nullptr; }
    
    template<typename String, typename Fun>
    void update(String, Fun f)
    {
      f(p);
    }
};

TEST_CASE( "message handler", "[ossia][message_handler]" ) {
  {
    // Create a message
    const char message[]{
      '/', 'a', 0,  0, 
      ',', 'i', 0,  0, 
       0,   0,  0, 64};
    oscpack::ReceivedMessage m(oscpack::ReceivedPacket{message, 12});
    
    // Value of the message
    std::cerr << m.ArgumentCount() << " argument: " << m.ArgumentsBegin()->AsInt32() << "\n";
    
    // Mock device
    mock_device d;
    d.p.variants.push_back(int32_t{0});
    
    // Test the parsing
    coppa::ossia::message_handler::on_messageReceived(d, m);
    
    // Check parsing result
    auto val = d.p.variants[0];
    REQUIRE(which(val) == Type::int_t);
    REQUIRE(get<int32_t>(val) == 64);
  }
  
  {
    float f = 42;
    char f1 = reinterpret_cast<char*>(&f)[0];
    char f2 = reinterpret_cast<char*>(&f)[1];
    char f3 = reinterpret_cast<char*>(&f)[2];
    char f4 = reinterpret_cast<char*>(&f)[3];
    // Create a message
    const char message[]{
      '/', 'a',  0 ,  0 , 
      ',', 'i', '[', 'f', 
      'f', ']', 'i',  0 ,
       0 ,  0 ,  0 ,  9 , // i 
      f4 , f3 , f2 , f1 , // [ f
      f4 , f3 , f2 , f1 , // f ]
       0 ,  0 ,  0 ,  9 , // i 
    };
    oscpack::ReceivedMessage m(oscpack::ReceivedPacket{message, 28});
    
    // Value of the message
    std::cerr << m.ArgumentCount() << " argument: " << m.ArgumentsBegin()->AsInt32() << "\n";
    
    // Mock device
    mock_device d;
    d.p.variants.push_back(int32_t{0});
    d.p.variants.push_back(Tuple{0.0f, 0.0f});
    d.p.variants.push_back(int32_t{0});
    
    // Test the parsing
    coppa::ossia::message_handler::on_messageReceived(d, m);
    
    // Check parsing result
    {
      auto val = d.p.variants[0];
      REQUIRE(which(val) == Type::int_t);
      REQUIRE(get<int32_t>(val) == 9);
    }
    {
      auto val = d.p.variants[1];
      REQUIRE(which(val) == Type::tuple_t);
      auto tpl = Tuple{f, f};
      REQUIRE(get<Tuple>(val) == tpl);
    } 
    {
      auto val = d.p.variants[2];
      REQUIRE(which(val) == Type::int_t);
      REQUIRE(get<int32_t>(val) == 9);
    }
  }
}

#include <functional>

TEST_CASE( "device", "[ossia][osc_local_device]" ) {
  osc_local_impl::map_type map;
  
  osc_local_impl test(map, 1234);
  auto cb = [] (const Parameter& param) {
    std::cerr << param.destination << std::endl;
  };
  test.on_value_changed.connect(&cb);
  
  test.on_value_changed.disconnect(&cb);
}
