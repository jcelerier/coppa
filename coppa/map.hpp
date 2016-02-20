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
using ParameterMapType = 
bmi::multi_index_container<
  ParameterType,
  bmi::indexed_by<
    bmi::ordered_unique< // TODO compare performance with hashed_unique
      bmi::member<
        Destination,
        std::string,
        &Destination::destination>,
      std::less<>>,
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


/**
 * @brief The basic_map class
 *
 * A map type that is meant to work with address-based protocols.
 * Provides an always-existent root node.
 */
template<typename Map>
class basic_map
{
    Map m_map;
    static auto make_root_node()
    {
      typename Map::value_type root;
      root.description = std::string("root node");
      root.destination = std::string("/");
      root.access = Access::Mode::None;
      return root;
    }

    auto make_update_fun()
    {
      return [] (auto&& ) { };
    }
    template<typename Arg>
    auto make_update_fun(Arg&& arg)
    {
      return [&] (auto&& p) {
        static_cast<Arg&&>(p) = std::forward<Arg>(arg);
      };
    }

    template<typename Arg, typename... Args>
    auto make_update_fun(Arg&& arg, Args&&... args)
    {
      return [&] (auto&& p) {
        make_update_fun(std::forward<Args>(args)...)(p);
        static_cast<Arg&&>(p) = std::forward<Arg>(arg);
      };
    }

  public:
    using base_map_type = Map;
    using size_type = typename Map::size_type;
    using value_type = typename base_map_type::value_type;

    constexpr basic_map()
    { insert(make_root_node()); }

    basic_map& operator=(Map&& map)
    {
      m_map = std::move(map);
      return *this;
    }


    template<typename Key>
    auto find(Key&& address) const
    {
      return m_map.template get<0>().find(std::forward<Key>(address));
    }

    template<typename Key>
    bool has(Key&& address) const
    {
      decltype(auto) index = m_map.template get<0>();
      return index.find(std::forward<Key>(address)) != index.end();
    }


    template<typename Key>
    bool existing_path(Key&& address) const
    {
      return std::any_of(
            std::begin(m_map),
            std::end(m_map),
            [&] (const typename Map::value_type& param) {
        return boost::starts_with(
              param.destination, 
              std::forward<Key>(address)); 
      });
    }

    template<typename Key>
    auto get(Key&& address) const
    { return *m_map.template get<0>().find(address); }

    operator const Map&() const
    { return m_map; }

    const auto& map_impl() const
    { return static_cast<const Map&>(*this); }

    auto& operator[](size_type i) const
    { return m_map.template get<1>()[i]; }

    FORWARD_FUN_CONST(m_map, auto, size)
    FORWARD_FUN_CONST(m_map, auto, begin)
    FORWARD_FUN_CONST(m_map, auto, end)

    // TODO add insert_and_assign
    // See boost doc with rollback, too.
    FORWARD_FUN(m_map, auto, insert)

    template<typename Key,
             typename Updater>
    auto update(Key&& address, Updater&& updater)
    {
      auto& param_index = m_map.template get<0>();
      auto it = param_index.find(std::forward<Key>(address));
      if(it != param_index.end())
        return param_index.modify(it, std::forward<Updater>(updater));
      return false;
    }

    template<typename Key,
             typename... Args>
    auto update_attributes(Key&& address, Args&&... args)
    {
      return update(std::forward<Key>(address),
                 make_update_fun(std::forward<Args>(args)...));
    }

    template<typename Element>
    auto replace(const Element& replacement)
    {
      auto& param_index = m_map.template get<0>();
      auto it = param_index.find(replacement.destination);
      if(it != param_index.end())
        return param_index.replace(it, replacement);
      return false;
    }

    template<typename Key>
    auto remove(Key&& k)
    {
      // Remove the path and its children
      for(auto&& elt : filter(*this, std::forward<Key>(k)))
      {
        m_map.template get<0>().erase(elt.destination);
      }

      // If the root node was removed we reinstate it
      if(size() == 0)
      {
        insert(make_root_node());
      }
    }

    template<typename Map_T>
    void merge(Map_T&& other)
    {
      // TODO OPTIMIZEME
      for(auto&& elt : std::forward<Map_T>(other))
      {
        if(has(elt.destination))
          replace(elt);
        else
          insert(elt);
      }
    }
};


template<typename Map>
/**
 * @brief The locked_map class
 *
 * Thread-safe wrapper for maps
 */
class locked_map
{
    mutable boost::shared_mutex m_map_mutex;
    Map& m_map;

  public:
    using data_map_type = Map;
    using parent_map_type = Map;
    using base_map_type = typename Map::base_map_type;
    using value_type = typename base_map_type::value_type;

    constexpr locked_map(Map& source):
      m_map{source}
    {
      
    }

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
    auto begin() { 
      auto&& l = acquire_read_lock();
      return m_map.begin();
    }
    auto end() { 
      auto&& l = acquire_read_lock();
      return m_map.end();
    }
    auto begin() const { 
      auto&& l = acquire_read_lock();
      return const_cast<const Map&>(m_map).begin();
    }
    auto end() const { 
      auto&& l = acquire_read_lock();
      return const_cast<const Map&>(m_map).end();
    }
    
    template<typename K>
    auto find(K&& k) { 
      auto&& l = acquire_read_lock();
      return m_map.find(std::forward<K>(k));
    }
    
    template<typename K>
    auto find(K&& k) const { 
      auto&& l = acquire_read_lock();
      return const_cast<const Map&>(m_map).find(std::forward<K>(k));
    }

    // operator[] to be locked explicitly from the outside since
    // it returns a ref.
    auto& operator[](typename Map::size_type i) 
    { return m_map[i]; }
    auto& operator[](typename Map::size_type i) const
    { return const_cast<const Map&>(m_map)[i]; }

    // These are of course unsafe, too
    data_map_type& get_data_map() 
    { return static_cast<data_map_type&>(m_map); }
    const data_map_type& get_data_map() const 
    { return static_cast<const data_map_type&>(m_map); }


    auto size() const
    {
      auto&& l = acquire_read_lock();
      return m_map.size();
    }

    template<typename Map_T>
    locked_map& operator=(Map_T&& map)
    {
      auto&& l = acquire_write_lock();
      m_map = std::forward<Map_T>(map);
      return *this;
    }

    template<typename Key>
    bool has(Key&& address) const
    {
      auto&& l = acquire_read_lock();
      return m_map.has(std::forward<Key>(address));
    }

    template<typename Key>
    bool existing_path(Key&& address) const
    {
      auto&& l = acquire_read_lock();
      return m_map.existing_path(std::forward<Key>(address));
    }

    template<typename Key>
    auto get(Key&& address) const
    {
      auto&& l = acquire_read_lock();
      return m_map.get(std::forward<Key>(address));
    }

    template<typename... Args>
    auto update(Args&&... args)
    {
      auto&& l = acquire_write_lock();
      return m_map.update(std::forward<Args>(args)...);
    }

    template<typename... Args>
    auto update_attributes(Args&&... args)
    {
      auto&& l = acquire_write_lock();
      return m_map.update_attributes(std::forward<Args>(args)...);
    }

    template<typename... Args>
    auto replace(Args&&... args)
    {
      auto&& l = acquire_write_lock();
      return m_map.replace(std::forward<Args>(args)...);
    }

    template<typename Element>
    void insert(Element&& e)
    {
      auto&& l = acquire_write_lock();
      m_map.insert(std::forward<Element>(e));
    }

    template<typename Key>
    void remove(Key&& k)
    {
      auto&& l = acquire_write_lock();
      m_map.remove(std::forward<Key>(k));
    }

    template<typename Map_T>
    void merge(Map_T&& other)
    {
      auto&& l = acquire_write_lock();
      m_map.merge(std::forward<Map_T>(other));
    }
};


/**
 * @brief The constant_map class
 *
 * This wrapper is meant to have a safe map that has no modifying accessor
 */
template<typename Map>
class constant_map_view
{
  public:
    constant_map_view(Map& map):
      m_map{map}
    {
      
    }
    
    template<typename K>
    auto has(K&& k) const 
    { return m_map.has(std::forward<K>(k)); }
    
    template<typename K>
    auto get(K&& k) const 
    { return m_map.get(std::forward<K>(k)); }
    
    auto size() const
    { return m_map.size(); }
    
    template<typename Map_T>
    void replace(Map_T&& map)
    { m_map = std::move(map); }
    
  private:
    Map& m_map;
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
class remote_map_setter 
{
  private:
    DataProtocolSender m_sender;
    Map& m_map;

  public:
    remote_map_setter(Map& map): 
      m_map{map}
    {
      
    }
    
    auto& map() 
    { return m_map; }
    auto& map() const  
    { return m_map; }
    
    void connect(const std::string& uri, int port)
    { m_sender = DataProtocolSender{uri, port}; }

    template<typename... Args>
    void set(const std::string& address, Args&&... args)
    {
      auto param = m_map.get(address);
      if(param.access == Access::Mode::Set
      || param.access == Access::Mode::Both)
      {
        m_sender.send(address, std::forward<Args>(args)...);
      }
    }
};
}
