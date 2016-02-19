#pragma once
#include <mutex>
#define BOOST_SYSTEM_NO_DEPRECATED
#include <boost/thread/shared_mutex.hpp>

#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>

#include <type_traits>
namespace coppa
{
// We make maps on parameter with a destination
namespace bmi = boost::multi_index;
template<typename ParameterType>
using ParameterMapType = bmi::multi_index_container<
ParameterType,
bmi::indexed_by<
bmi::ordered_unique< // TODO compare performance with hashed_unique
bmi::member<
Destination,
std::string,
&Destination::destination>>,
bmi::random_access<>>>;

// Get all the parameters whose address begins with addr
// TODO make an algorithm to rebase a map with a new root. (the inverse of this)
template<typename Map, typename Key>
auto filter(const Map& map, Key&& addr)
{
  typename Map::base_map_type newmap;
  for(const auto& param : map)
  {
    if(boost::starts_with(param.destination, addr))
      newmap.insert(param);
  }

  return newmap;
}

template<typename Map>
/**
 * @brief The locked_map class
 *
 * Thread-safe wrapper for maps
 */
class locked_map : private Map
{
    mutable boost::shared_mutex m_map_mutex;

  public:
    using data_map_type = Map;
    using parent_map_type = Map;
    using base_map_type = typename Map::base_map_type;
    using value_type = typename base_map_type::value_type;

    constexpr locked_map() = default;

    boost::shared_lock<boost::shared_mutex> acquire_read_lock() const
    {
      return boost::shared_lock<boost::shared_mutex>(m_map_mutex);
    }
    boost::unique_lock<boost::shared_mutex> acquire_write_lock()
    {
      return boost::unique_lock<boost::shared_mutex>(m_map_mutex);
    }

    // Note : these iterators are here for convenience purpose.
    // However the map has to be locked manually when using them with
    // acquire_r/w_lock

    // TODO would it be possible to have iterators wrapped so that
    // they would "carry" a lock with them ?
    using Map::begin;
    using Map::end;
    using Map::find;

    // operator[] to be locked explicitly from the outside since
    // it returns a ref.
    auto& operator[](typename Map::size_type i) const
    { return Map::operator[](i); }

    // These are of course unsafe, too
    auto& get_data_map() { return static_cast<data_map_type&>(*this); }
    auto& get_data_map() const { return static_cast<const data_map_type&>(*this); }


    auto size() const
    {
      auto&& l = acquire_read_lock();
      return Map::size();
    }

    template<typename Map_T>
    locked_map& operator=(Map_T&& map)
    {
      auto&& l = acquire_write_lock();
      static_cast<Map&>(*this) = std::move(map);
      return *this;
    }

    template<typename Key>
    bool has(Key&& address) const
    {
      auto&& l = acquire_read_lock();
      return Map::has(address);
    }

    template<typename Key>
    bool existing_path(Key&& address) const
    {
      auto&& l = acquire_read_lock();
      return Map::existing_path(address);
    }

    template<typename Key>
    auto get(Key&& address) const
    {
      auto&& l = acquire_read_lock();
      return Map::get(address);
    }

    template<typename... Args>
    void update(Args&&... args)
    {
      auto&& l = acquire_write_lock();
      Map::update(std::forward<Args>(args)...);
    }

    template<typename... Args>
    void update_attributes(Args&&... args)
    {
      auto&& l = acquire_write_lock();
      Map::update_attributes(std::forward<Args>(args)...);
    }

    template<typename... Args>
    void replace(Args&&... args)
    {
      auto&& l = acquire_write_lock();
      Map::replace(std::forward<Args>(args)...);
    }

    template<typename Element>
    void insert(Element&& e)
    {
      auto&& l = acquire_write_lock();
      Map::insert(e);
    }

    template<typename Key>
    void remove(Key&& k)
    {
      auto&& l = acquire_write_lock();
      Map::remove(k);
    }

    template<typename Map_T>
    void merge(Map_T&& other)
    {
      auto&& l = acquire_write_lock();
      Map::merge(std::move(other));
    }
};


/**
 * @brief The constant_map class
 *
 * This wrapper is meant to have a safe map that has no modifying accessor
 */
template<typename Map>
class constant_map : private locked_map<Map>
{
  public:
    using parent_map_type = locked_map<Map>;
    using data_map_type = typename locked_map<Map>::data_map_type;
    auto& get_locked_map() { return static_cast<parent_map_type&>(*this); }
    auto& get_locked_map() const { return static_cast<parent_map_type&>(*this); }
    auto& get_data_map() { return parent_map_type::get_data_map(); }
    auto& get_data_map() const { return parent_map_type::get_data_map(); }

    using parent_map_type::has;
    using parent_map_type::get;
    using parent_map_type::size;

    template<typename Map_T>
    void replace(Map_T&& map)
    { get_locked_map() = std::move(map); }
};

/**
 * @brief The remote_map_setter class
 *
 * This class uses a constant_map as backend, and allows updating
 * of a *remote* map thanks to the information it has.
 *
 * The local map would then only be updated on a callback from
 * the remote server.
 *
 * TODO make alternatives :
 * - Also sets the local map
 */
template<typename Map, typename DataProtocolSender>
class remote_map_setter : public constant_map<Map>
{
  private:
    DataProtocolSender m_sender;

  public:
    void connect(const std::string& uri, int port)
    { m_sender = DataProtocolSender{uri, port}; }

    template<typename... Args>
    void set(const std::string& address, Args&&... args)
    {
      auto param = this->get(address);
      if(param.accessmode == Access::Mode::Set
      || param.accessmode == Access::Mode::Both)
      {
        m_sender.send(address, std::forward<Args>(args)...);
      }
    }
};
}
