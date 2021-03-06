#pragma once
#include <coppa/minuit/parameter.hpp>
#include <iostream>

namespace coppa
{
namespace minuit
{
inline std::ostream& operator <<(std::ostream& stream, const coppa::minuit::Variant& v)
{
  using namespace eggs::variants;
  struct vis {
      std::ostream& s;
      void operator()(None val) const {
        s << "None ";
      }
      void operator()(Impulse val) const {
        s << "Impulse ";
      }

      void operator()(int32_t val) const {
        s << "Int: " << val << " ";
      }
      void operator()(float val) const {
        s << "Float: " << val << " ";
      }
      void operator()(bool val) const {
        s << "Bool: " << val << " ";
      }
      void operator()(char val) const {
        s << "Char: " << val << " ";
      }
      void operator()(const std::string& val) const {
        s << "Str: " << val << " ";
      }
      void operator()(const Tuple& t) const {
        s << "Tuple ";
      }
      void operator()(const Generic& val) const {
        s << "Generic ";
      }

  } visitor{stream};

  eggs::variants::apply(visitor, v);
  return stream;
}

inline std::ostream& operator <<(std::ostream& stream, const coppa::minuit::Values& param)
{
  for(auto sub : param.variants)
  {
    stream << sub;
  }
  return stream;
}
inline std::ostream& operator <<(std::ostream& stream, const coppa::minuit::Parameter& param)
{
  stream << param.destination << "\n"
         << static_cast<const coppa::minuit::Values&>(param) << "\n";

  return stream;
}
}
}
