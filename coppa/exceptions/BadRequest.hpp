#pragma once
#include <stdexcept>
class BadRequestException: public std::domain_error
{
  public:
    BadRequestException():
      std::domain_error{"Bad request"} { }

    BadRequestException(const std::string& message):
      std::domain_error{"Bad request : " + message} { }
};
