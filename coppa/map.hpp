#pragma once
#include <mutex>

#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
namespace coppa
{
// We make maps on parameter with a destination
namespace bmi = boost::multi_index;
template<typename ParameterType>
using ParameterMapType = bmi::multi_index_container<
    ParameterType,
    bmi::indexed_by<
        bmi::hashed_unique<
            bmi::member<
                Destination,
                std::string,
                &Destination::destination>>>>;

// Get all the parameters whose address begins with addr
template<typename MapType, typename Key>
auto filter(const MapType& map, Key&& addr)
{
    MapType newmap;
    for(const auto& param : map)
    {
        if(boost::starts_with(param.destination, addr))
            newmap.insert(param);
    }
    return newmap;
}

// TODO rebase with new root algorithm

template<typename Map>
class LockedParameterMap
{
    Map m_map;
    mutable std::mutex m_map_mutex;

  public:
    constexpr LockedParameterMap() { }

    LockedParameterMap& operator=(Map&& map)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map = std::move(map);
      return *this;
    }

    template<typename Key>
    bool has(Key&& address) const
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      decltype(auto) index = m_map.template get<0>();
      return index.find(address) != end(index);
    }

    template<typename Key>
    auto get(Key&& address) const
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      return *m_map.template get<0>().find(address);
    }

    constexpr operator Map() const
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      return m_map;
    }

    template<typename Key, typename Updater>
    void update(Key&& address, Updater&& updater)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      auto& param_index = m_map.template get<0>();
      decltype(auto) param = param_index.find(address);
      {
        param_index.modify(param, updater);
      }
    }
};

}
