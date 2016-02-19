#pragma once
#include <string>
#include <map>
#include <vector>
#include <boost/container/static_vector.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/algorithm/string.hpp>
#include <coppa/exceptions/BadRequest.hpp>
#include <coppa/string_view.hpp>
namespace coppa
{
namespace oscquery
{
   /**
   * @brief The query_parser class
   *
   * Parse OSCQUery queries.
   * The queries are similar to the GET part of an http request.
   * i.e. /a/b?value, etc...
   */
  class query_parser
  {
    public:
      template<typename Mapper>
      static std::string parse(const std::string& request, Mapper&& mapper)
      {
        using namespace boost;
        using boost::container::small_vector;
        using boost::container::static_vector;
        using namespace std;

        // Split on "?" 
        static_vector<std::string, 3> uri_tokens;
        try {
          split(uri_tokens, request, is_any_of("?"));
        }
        catch(...) {
          throw BadRequestException{"Too much '?'"};          
        }

        // Parse the methods
        map<std::string, std::string> arguments_map;
        if(uri_tokens.size() > 1)
        {
          // Then, split the &-separated arguments
          std::vector<string> argument_tokens;
          boost::split(argument_tokens, uri_tokens.at(1), is_any_of("&"));

          // Finally, split these arguments at '=' and put them in a map
          for(const auto& arg : argument_tokens)
          {
            static_vector<string, 3> map_tokens;
            
            try {
              boost::split(map_tokens, arg, is_any_of("="));
            }
            catch(...) {
              throw BadRequestException{"Too much '='"};          
            }

            switch(map_tokens.size())
            {
              case 1: // &value
                arguments_map.insert({map_tokens.front(), {}});
                break;
              case 2: // &listen=true
                arguments_map.insert({map_tokens.front(), map_tokens.back()});
                break;
              default:
                throw BadRequestException{"Too much '='"};
                break;
            }
          }
        }

        return mapper(uri_tokens.at(0), arguments_map);
      }

  };
}
}
