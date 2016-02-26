#pragma once
#include <string>
#include <coppa/string_view.hpp>

namespace coppa
{
namespace ossia
{


class MinuitProtocol
{
  public:
    Minuit(std::string, int, int);

    const std::string& getIp();
    void setIp(const std::string&);

    int getInPort() const;
    void setInPort(int);

    int getOutPort() const;
    void setOutPort(int);

  private:
    std::string ip;
    int inPort;
    int outPort;
};

class MinuitDevice
{

    void addNode(const std::string&);
    void removeNode(const std::string&);
    void renameNode(const std::string&, const std::string&);

    void setAttribute();
    void updateNamespace();

    void pushValue(const std::string&);
    void pullValue(const std::string&);

    void getValue(const std::string&);
    void setValue(const std::string&);
};


}
}
