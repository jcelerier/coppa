#pragma once
#include <nano-signal-slot/nano_signal_slot.hpp>
#include <coppa/ossia/parameter.hpp>
#include <unordered_map>

namespace coppa
{
namespace ossia
{

class device_with_callbacks
{
    public:
        device_with_callbacks()
        {
            on_value_changed.connect<device_with_callbacks, &device_with_callbacks::callback_helper>(this);
        }

        // Remote -> local
        Nano::Signal<void(Parameter)> on_value_changed;

        auto& get_value_callback(const std::string& dest)
        {
          return m_callbacks[dest];
        }

        template<typename Arg, typename... TArgs>
        void add_value_callback(const std::string& dest, const Arg& arg)
        {
          m_callbacks[dest].template connect<TArgs...>(arg);
        }

        template<typename Arg, typename... TArgs>
        void remove_value_callback(const std::string& dest, const Arg& arg)
        {
          m_callbacks[dest].template disconnect<TArgs...>(arg);
        }

    private:
        void callback_helper(Parameter p)
        {
          auto it = m_callbacks.find(p.destination);
          if(it != m_callbacks.end())
          {
            it->second(p);
          }
        }

        std::unordered_map<std::string, Nano::Signal<void(coppa::ossia::Value)>> m_callbacks;
};

}
}
