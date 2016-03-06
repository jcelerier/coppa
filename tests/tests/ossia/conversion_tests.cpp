#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <coppa/minuit/device/osc_common.hpp>
#include <coppa/protocol/osc/oscmessagegenerator.hpp>
using namespace coppa;
using namespace coppa::minuit;
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


TEST_CASE( "Convert, strict error", "[ossia][conversion]" ) {

  using err = strict_error_handler;
  const constexpr conversion_mode cv = conversion_mode::Convert;

  // Same type : expected behaviour
  {
    value_maker<err, cv> conv;
    Message message(4321);
    Variant v = 1;

    conv(message.mess.ArgumentsBegin(), v);

    REQUIRE(which(v) == Type::int_t);
    REQUIRE(get<int32_t>(v) == 4321);
  }

  // Convertible type : it gets converted
  {
    value_maker<err, cv> conv;
    Message message(4321.321);
    Variant v = 1;

    conv(message.mess.ArgumentsBegin(), v);

    REQUIRE(which(v) == Type::int_t);
    REQUIRE(get<int32_t>(v) == 4321);
  }

  // Non-convertible type : an exception is thrown
  {
    value_maker<err, cv> conv;
    Message message("a string");
    Variant v = 1;
    Variant expected = v;

    REQUIRE_THROWS_AS(
          conv(message.mess.ArgumentsBegin(), v),
          InvalidInputException);
    REQUIRE(v == expected);
  }

  // Multiple inputs, correct types : everything is converted
  {
    values_reader<err, cv> conv;
    Message message(4, 5, 6);
    Values v{1, 2, 3};

    conv(message.mess.ArgumentsBegin(), message.mess.ArgumentsEnd(), v);

    Values expected{4, 5, 6};
    REQUIRE(v.variants == expected.variants);
  }

  // Multiple inputs, wrong but convertible types : the convertible types
  // are converted
  {
    values_reader<err, cv> conv;
    Message message(4, 5, 0);
    Values v{1.0f, true, true};

    conv(message.mess.ArgumentsBegin(), message.mess.ArgumentsEnd(), v);

    Values expected{4.0f, true, false};
    REQUIRE(v.variants == expected.variants);
  }

  // Too many inputs : remaining are ignored
  {
    values_reader<err, cv> conv;
    Message message(1, 2, 3);
    Values v{0.0f, 0};

    Values expected{1.f, 2};
    conv(message.mess.ArgumentsBegin(), message.mess.ArgumentsEnd(), v);

    REQUIRE(v.variants == expected.variants);
  }

  // Not enough inputs : only the first ones are replaced
  {
    values_reader<err, cv> conv;
    Message message(32);
    Values v{1, 2, 3};

    conv(message.mess.ArgumentsBegin(), message.mess.ArgumentsEnd(), v);

    Values expected{32, 2, 3};
    REQUIRE(v.variants == expected.variants);
  }
}
TEST_CASE( "StrictConvert, lax error", "[ossia][conversion]" ) {
}

TEST_CASE( "StrictIgnore, strict error", "[ossia][conversion]" ) {
}
TEST_CASE( "StrictIgnore, lax error", "[ossia][conversion]" ) {
}

TEST_CASE( "StrictPreChecked, strict error", "[ossia][conversion]" ) {
}
TEST_CASE( "StrictPreChecked, lax error", "[ossia][conversion]" ) {
}


TEST_CASE( "Replace, strict error", "[ossia][conversion]" ) {
  using err = strict_error_handler;
  const constexpr conversion_mode cv = conversion_mode::Replace;

  {
    value_maker<err, cv> conv;
    Message message(4321);
    Variant v = 1;

    conv(message.mess.ArgumentsBegin(), v);

    Variant expected = 4321;
    REQUIRE(v == expected);
  }

  {
    value_maker<err, cv> conv;
    Message message(4321.f);
    Variant v = 1.f;

    conv(message.mess.ArgumentsBegin(), v);

    Variant expected{4321.f};
    REQUIRE(v == expected);
  }

  {
    value_maker<err, cv> conv;
    Message message(4321.f);
    Variant v = std::string("foo");

    conv(message.mess.ArgumentsBegin(), v);

    Variant expected{4321.f};
    REQUIRE(v == expected);
  }

  {
    values_reader<err, cv> conv;
    Message message(4321.f, 123.f, 12.f);
    Values v{1.f};

    conv(message.mess.ArgumentsBegin(), message.mess.ArgumentsEnd(), v);

    Values expected{4321.f, 123.f, 12.f};
    REQUIRE(v.variants == expected.variants);
  }

  {
    values_reader<err, cv> conv;
    Message message(4321.f, 123.f, 12.f);
    Values v{"foo", "bar", "baz", "bingo"};

    conv(message.mess.ArgumentsBegin(), message.mess.ArgumentsEnd(), v);

    Values expected{4321.f, 123.f, 12.f};
    REQUIRE(v.variants == expected.variants);
  }
}
TEST_CASE( "Replace, lax error", "[ossia][conversion]" ) {
}


TEST_CASE( "ossia mode", "[ossia][conversion]" ) {
  // In the future, as an optimization.
  // For now the OSSIA API can do the behaviour it wants on top of this.
}
