#include <exception>

#include "chain.h"

namespace modpro {

chain::chain(const std::string name_in) : name(name_in)
{

}

void chain::activate()
{
    for(auto i : effect_instances) {
        i.second->activate();
    }
}

void chain::run(const effect::size_type sample_count_in)
{
    for(auto i : run_list) {
        i->run(sample_count_in);
    }
}

void chain::add_effect(const std::string name_in, std::shared_ptr<effect> effect_in)
{
    if (effect_instances.count(name_in) != 0) {
        throw std::runtime_error("attempt to add duplicate effect name: " + name_in);
    }

    effect_instances[name_in] = effect_in;
    run_list.push_back(effect_in);
}

std::shared_ptr<effect> chain::get_effect(const std::string name_in)
{
    if (effect_instances.count(name_in) == 0) {
        throw std::runtime_error("effect name is not known: " + name_in);
    }

    return effect_instances[name_in];
}

void chain::add_route(std::string port_name_in, std::shared_ptr<jackaudio::audio_port> port_in)
{
    jack_connections.push_back(std::make_pair(port_name_in, port_in));
}

const std::vector<std::pair<std::string, std::shared_ptr<jackaudio::audio_port>>> chain::get_routes()
{
    return jack_connections;
}

}
