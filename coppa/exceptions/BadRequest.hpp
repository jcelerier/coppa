#pragma once
#include <stdexcept>
class PathNotFoundException: public std::domain_error
{
  public:
    PathNotFoundException():
      std::domain_error{"Requested path not found"} { }

    PathNotFoundException(const std::string& message):
      std::domain_error{"Requested path not found : " + message} { }
};

class BadRequestException: public std::domain_error
{
  public:
    BadRequestException():
      std::domain_error{"Bad request"} { }

    BadRequestException(const std::string& message):
      std::domain_error{"Bad request : " + message} { }
};

class InvalidInputException: public std::domain_error
{
  public:
    InvalidInputException():
      std::domain_error{"Invalid input"} { }

    InvalidInputException(const std::string& message):
      std::domain_error{"Invalid input : " + message} { }
};

