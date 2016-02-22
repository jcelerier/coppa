#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <coppa/minuit/device/osc_common.hpp>
#include <coppa/protocol/osc/oscmessagegenerator.hpp>
using namespace coppa;
using namespace coppa::ossia;
using namespace eggs::variants;

struct Message
{
    oscpack::MessageGenerator<> gen;
    oscpack::ReceivedMessage mess;

    template<typename... Args>
    Message(Args&&... args):
       gen{"/foo", std::forward<Args>(args)...},
       mess{oscpack::ReceivedPacket(gen.stream().Data(), gen.stream().Capacity())}
    {

    }

    operator oscpack::ReceivedMessage() const {
      return mess;
    }
};


TEST_CASE( "StrictConvert, strict error", "[ossia][conversion]" ) {
  ValueMaker<strict_error_handler, ConversionMode::StrictConvert> conv;


  // Same type : expected behaviour
  {
    Message message(4321);
    Variant v = 1;

    conv(message.mess.ArgumentsBegin(), v);

    REQUIRE(which(v) == Type::int_t);
    REQUIRE(get<int32_t>(v) == 4321);
  }

  // Convertible type : it gets converted
  {
    Message message(4321.321);
    Variant v = 1;

    conv(message.mess.ArgumentsBegin(), v);

    REQUIRE(which(v) == Type::int_t);
    REQUIRE(get<int32_t>(v) == 4321);
  }

  {
    Message message(1, 2, 3);
    Variant v;

    REQUIRE_THROWS_AS(
          conv(message.mess.ArgumentsBegin(), v),
          eggs::variants::bad_variant_access);
  }

}
TEST_CASE( "StrictConvert, lax error", "[ossia][conversion]" ) {
  ValueMaker<lax_error_handler, ConversionMode::StrictConvert> conv;

}

TEST_CASE( "StrictIgnore, strict error", "[ossia][conversion]" ) {
  ValueMaker<strict_error_handler, ConversionMode::StrictIgnore> conv;

}
TEST_CASE( "StrictIgnore, lax error", "[ossia][conversion]" ) {
  ValueMaker<lax_error_handler, ConversionMode::StrictIgnore> conv;

}

TEST_CASE( "StrictPreChecked, strict error", "[ossia][conversion]" ) {
  ValueMaker<strict_error_handler, ConversionMode::StrictPreChecked> conv;

}
TEST_CASE( "StrictPreChecked, lax error", "[ossia][conversion]" ) {
  ValueMaker<lax_error_handler, ConversionMode::StrictPreChecked> conv;

}


TEST_CASE( "Replace, strict error", "[ossia][conversion]" ) {
  ValueMaker<strict_error_handler, ConversionMode::Replace> conv;

}
TEST_CASE( "Replace, lax error", "[ossia][conversion]" ) {
  ValueMaker<lax_error_handler, ConversionMode::Replace> conv;

}
