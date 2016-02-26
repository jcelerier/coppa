#pragma once
#include <coppa/string_view.hpp>
#include <coppa/minuit/device/minuit_common.hpp>

namespace coppa
{
namespace ossia
{

class minuit_name_table
{
  public:
    minuit_name_table(const std::string& other)
    {
      set_device_name(other);
    }

    string_view get_device_name() const
    { return m_name; }

    void set_device_name(const std::string& name)
    {
      set_device_name(name);

      m_actions[(int)minuit_action::NamespaceRequest] = name + "?namespace";
      m_actions[(int)minuit_action::NamespaceReply]   = name + ":namespace";
      m_actions[(int)minuit_action::NamespaceError]   = name + "!namespace";

      m_actions[(int)minuit_action::GetRequest] = name + "?get";
      m_actions[(int)minuit_action::GetReply]   = name + ":get";
      m_actions[(int)minuit_action::GetError]   = name + "!get";

      m_actions[(int)minuit_action::ListenRequest] = name + "?listen";
      m_actions[(int)minuit_action::ListenReply]   = name + ":listen";
      m_actions[(int)minuit_action::ListenError]   = name + "!listen";
    }

    string_view get_action(minuit_action c)
    { return m_actions[static_cast<int>(c)]; }

  private:
    std::string m_name;
    std::array<std::string, 9> m_actions;
};
}
}
