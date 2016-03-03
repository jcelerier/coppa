#include <coppa/minuit/device/local.hpp>
#include <coppa/minuit/device/minuit_remote_behaviour.hpp>
#include <thread>
using namespace coppa;
using namespace coppa::ossia;

struct GetPromise
{
        std::string address;
        std::promise<coppa::ossia::Parameter> promise;
};
namespace coppa
{
namespace ossia
{


template<
        template<
          minuit_command,
          minuit_operation>
        class Handler,
        minuit_command Req,
        minuit_operation Op>
struct minuit_callback_behaviour_wrapper
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
        Handler<Req, Op>{}(dev, map, mess);
    }
};


template<
        template<
          minuit_command,
          minuit_operation>
        class Handler>
struct minuit_callback_behaviour_wrapper<Handler, minuit_command::Answer, minuit_operation::Get>
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
        auto res_it = Handler<minuit_command::Answer, minuit_operation::Get>{}(dev, map, mess);

        if(res_it != map.end())
        {
            auto prom_it = std::find_if(
                        dev.m_getPromises.begin(),
                        dev.m_getPromises.end(),
                        [=] (const auto& prom) {
               return prom.address == res_it->destination;
            });

            if(prom_it != dev.m_getPromises.end())
            {
              prom_it->promise.set_value(*res_it);
            }
        }
        // TODO here : - boost.lockfree queue for namespaces : we add the requested paths to a queue
        // when they are received we remove them
        // when the queue is empty, the namespace querying is finished (else we retry three times or something)

        // For "get" we set our promise as soon as we're done ? where shall we store them ?

        //dev.m_getPromises
    }
};

template<
  minuit_command c,
  minuit_operation op>
using minuit_callback_behaviour_wrapper_t = minuit_callback_behaviour_wrapper<minuit_remote_behaviour, c, op>;
template<
        template<
          minuit_command,
          minuit_operation>
        class Handler>
struct minuit_callback_behaviour_wrapper<Handler, minuit_command::Answer, minuit_operation::Listen>
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
        Handler<minuit_command::Answer, minuit_operation::Listen>{}(dev, map, mess);
    }
};

template<
        template<
          minuit_command,
          minuit_operation>
        class Handler>
struct minuit_callback_behaviour_wrapper<Handler, minuit_command::Answer, minuit_operation::Namespace>
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
        Handler<minuit_command::Answer, minuit_operation::Namespace>{}(dev, map, mess);
    }
};
class minuit_remote_impl_future : public osc_local_device<
    locked_map<basic_map<ParameterMapType<Parameter>>>,
    osc::receiver,
    minuit_message_handler<minuit_callback_behaviour_wrapper_t>,
    osc::sender>
{
  public:

    minuit_remote_impl_future(
        const std::string& name,
        map_type& map,
        unsigned int in_port,
        std::string out_ip,
        unsigned int out_port):
      osc_local_device{map, in_port, out_ip, out_port,
                       [&] (const auto& m, const auto& ip) {
      data_handler_t::on_messageReceived(*this, this->map(), m, ip);
    }},
      nameTable{name}
    {

    }

    auto get(const std::string& txt)
    {
        auto act = nameTable.get_action(minuit_action::GetRequest);
        this->sender.send(act, string_view(txt));

        m_getPromises.emplace_back();
        return m_getPromises.back().promise.get_future();
    }

    minuit_name_table nameTable;

    std::vector<GetPromise> m_getPromises;
};

}
}
auto make_parameter(std::string name)
{
  Parameter p;
  p.destination = name;

  return p;
}

void refresh(minuit_remote_impl& remote)
{
  remote.map().clear();

  // first request namespace
  const char * root = "/";
  remote.sender.send(remote.name() + "?namespace", root);
}

int main()
{
  basic_map<ParameterMapType<Parameter>> base_map;
  locked_map<basic_map<ParameterMapType<Parameter>>> map(base_map);

  minuit_remote_impl remote("rimoute", map, 13579, "127.0.0.1", 9998);
  refresh(remote);

  auto test_functor = [] (const Parameter& p) {
    std::cerr << p.destination << std::endl;
  };

  remote.on_value_changed.connect(test_functor);

  std::this_thread::sleep_for(std::chrono::seconds(1));
  const char * addr = "/tutu";
  auto sv = string_view(addr);
  auto pattern = remote.name() + "?get";
  auto sp = string_view(pattern);
  auto t1 = std::chrono::high_resolution_clock::now();
  for(int i = 0; i < 10000000; i++)
  {
    remote.sender.send(sv, i);
  }

  auto t2 = std::chrono::high_resolution_clock::now();
  std::cerr << "time " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << "\n";
}
